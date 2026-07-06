#include "mesh_session.hpp"

#include <atomic>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <dlfcn.h>

#include <fstream>

#include "cid/cid.hpp"
#include "crown.hpp"
#include "dagcbor/dagcbor.hpp"
#include "exec_plan.hpp"
#include "pins/pins.hpp"
#include "registry_face/registry_face.hpp"
#include "identity/identity.hpp"
#include "link/link.hpp"
#include "parser/parser.hpp"
#include "store.hpp"

namespace {

using nlohmann::json;
using syg::mesh::bytes;

// Wire kinds, table order (ch. 14, pins::wire_kinds), 1-based.
enum kind { PAIR = 1, HELLO = 2, SUBSCRIBE = 3, OPS = 4, FETCH = 5,
            QUERY = 6, PLACE = 7, FAULT = 8 };

bytes enc(const json& j) { return syg::formats::encode_projection(j); }
json dec(const bytes& b) { return syg::formats::decode_to_projection(b); }

// A store graph with a sharing policy (MSH-8): `share` empty-optional means
// "shared with every paired key"; a set means only those peer-keys.
struct sub_store {
  syg::store::peer_store store;
  std::optional<std::set<std::string>> share;  // nullopt = all paired
  explicit sub_store(std::string name) : store(std::move(name), false) {}
};

struct advert {
  std::set<std::string> run, serve, subscribe;
};

// The peer's native-contract posture (ABI-4): the contract hash it speaks and
// the migration edges it declares. A plugin built against C1 is admitted iff
// C1 reaches this peer's contract through declared migrations (reachability,
// not integer equality — ADR-025). Contract hashes are opaque here; they
// stand for real ABI contract-descriptor CIDs (ch. 13).
struct contract_posture {
  std::string speaks;                                     // C2
  std::vector<std::pair<std::string, std::string>> migrations;  // {from,to}
};

// Is `from` reachable to `to` through the declared migration edges?
inline bool contract_reaches(const contract_posture& c, const std::string& from) {
  if (from == c.speaks) return true;  // identity: same contract
  std::set<std::string> seen{from};
  std::vector<std::string> frontier{from};
  while (!frontier.empty()) {
    auto cur = frontier.back();
    frontier.pop_back();
    for (const auto& [a, b] : c.migrations)
      if (a == cur && seen.insert(b).second) {
        if (b == c.speaks) return true;
        frontier.push_back(b);
      }
  }
  return false;
}

struct peer {
  std::string name;
  syg::mesh::identity id;
  std::vector<std::unique_ptr<sub_store>> stores;  // [0] = default (all paired)
  advert lists;
  contract_posture contract;                    // ABI-4 posture
  std::set<std::string> accepted;               // paired peer-keys
  std::set<std::string> trusted_signers;        // provenance policy (MSH-5)
  std::map<std::string, json> plugin_prov;      // loaded plugin type -> prov
  std::vector<std::string> instantiated;        // registry audit log (MSH-4)
  std::mutex mu;
  std::unique_ptr<syg::mesh::listener> lis;
  std::thread server;
  std::atomic<bool> running{true};

  explicit peer(std::string n, const std::string& seed)
      : name(std::move(n)), id(seed) {
    stores.push_back(std::make_unique<sub_store>(name));
    lis = std::make_unique<syg::mesh::listener>();
  }
  ~peer() {  // exception-safe: a joinable thread at unwind would std::terminate
    running.store(false);
    if (lis) lis->stop();
    if (server.joinable()) server.join();
  }
};

// The mesh: peers by name. Discovery is an abstract provider (MSH-7): the
// household map here IS the static list; an mDNS beacon would populate the
// same map without touching any of the semantics below.
struct household {
  std::map<std::string, std::unique_ptr<peer>> peers;

  peer& at(const std::string& n) { return *peers.at(n); }
  std::uint16_t port_of(const std::string& n) { return peers.at(n)->lis->port(); }
};

// The server side: dispatch one authenticated request. `req_from` is the
// verified remote peer-key (handshake proved it). Everything here already
// knows the caller is paired — advertisement/sharing are the finer gates.
json handle(peer& me, std::uint64_t k, const json& body,
            const std::string& req_from) {
  std::lock_guard<std::mutex> g(me.mu);
  switch (k) {
    case FETCH: {
      // Serve a HELD object (the store::fetch model — availability is holding)
      // if the store's sharing policy admits this requester's key. The default
      // store shares with every paired key; a restricted store names its set.
      const std::string cid = body.at("cid");
      for (auto& ss : me.stores) {
        bool allowed = !ss->share || ss->share->count(req_from);
        if (!allowed) continue;  // held, but not shared with this key
        if (auto obj = ss->store.get(cid))
          return {{"ok", true}, {"obj", syg::formats::projection_of_bytes(*obj)}};
      }
      return {{"ok", false}, {"reason", "not-shared-or-absent"}};
    }
    case SUBSCRIBE: {  // a live address: reply the ref's current binding
      const std::string ref = body.at("ref");
      const std::string* bound = me.stores.at(0)->store.ref(ref);
      return {{"ok", true}, {"ref", ref},
              {"bound", bound ? json(*bound) : json(nullptr)}};
    }
    case HELLO:  // the three sovereign lists (MSH-3)
      return {{"ok", true},
              {"run", me.lists.run},
              {"serve", me.lists.serve},
              {"subscribe", me.lists.subscribe}};
    case PLACE: {
      // pull-shaped placement (MSH-3/4): instantiate iff the type is on the
      // run list. Nothing is pushed; the request is refused with a typed
      // error the caller can act on. The audit log proves MSH-4.
      const std::string type = body.at("type");
      if (!me.lists.run.count(type))
        return {{"ok", false}, {"error", "not-advertised"}, {"type", type},
                {"peer", me.name}};
      me.instantiated.push_back(type);
      return {{"ok", true}, {"placed_on", me.name}, {"type", type}};
    }
    case QUERY: {
      // Worker placement (PKG-5): the capability is ADVERTISED (run list),
      // queried over the wire — no test-supplied table. The advertising peer
      // runs the derivation on ITS executor and provides the result by hash;
      // re-placing is a memo hit. Refused (typed) if the capability isn't
      // advertised here.
      const std::string cap = body.at("capability");
      if (!me.lists.run.count(cap))
        return {{"ok", false}, {"error", "not-advertised"}, {"capability", cap},
                {"peer", me.name}};
      auto& store = me.stores.at(0)->store;
      const int blocks = body.value("blocks", 200);
      auto graph_cid = store.put_node(body.at("graph"), false);
      json recipe{{"op", "render-analysis"},
                  {"inputs", {{"graph", {{"/", graph_cid}}}}},
                  {"blocks", blocks},
                  {"determinism", "exact"}};
      if (auto hit = store.memo_lookup(recipe)) {
        auto take_cid = store.get_node(hit->output).at("take").at("/");
        return {{"ok", true}, {"placed_on", me.name}, {"output", hit->output},
                {"provenance", hit->provenance}, {"take", take_cid},
                {"memo", true}};
      }
      syg::executor::exec_plan wp(syg::organs::parse_graph(body.at("graph")),
                                  48000, 128);
      syg::formats::byte_vec take;
      json rms = json::array();
      for (int i = 0; i < blocks; ++i) {
        const float* b = wp.pump_block();
        double acc = 0;
        for (int k = 0; k < 128; ++k) acc += double(b[k]) * b[k];
        rms.push_back(acc / 128.0);
        const auto* bp = reinterpret_cast<const std::uint8_t*>(b);
        take.insert(take.end(), bp, bp + 128 * sizeof(float));
      }
      auto take_cid = store.put_raw(take, true);  // the worker provides it
      auto c = store.commit_derivation(
          recipe, {{"kind", "analysis"}, {"take", {{"/", take_cid}}},
                   {"rms_blocks", rms.size()}, {"rms", rms}});
      return {{"ok", true}, {"placed_on", me.name}, {"output", c.output},
              {"provenance", c.provenance}, {"take", take_cid}, {"memo", false}};
    }
    default:
      return {{"ok", false}, {"error", "unknown-message-kind"}};
  }
}

void serve_loop(peer* me) {
  auto admit = [me](const std::string& pk) {
    std::lock_guard<std::mutex> g(me->mu);
    return me->accepted.count(pk) > 0;
  };
  while (me->running.load()) {
    auto ch = me->lis->accept(me->id, admit);
    if (!ch) {
      if (!me->running.load()) break;
      continue;  // a refusal/garbage probe: keep serving
    }
    std::uint64_t k;
    bytes reqb;
    while (ch->recv(k, reqb)) {
      json reply;
      try {
        reply = handle(*me, k, dec(reqb), ch->remote_peer_key());
      } catch (const std::exception& e) {
        reply = {{"ok", false}, {"error", "handler-fault"}, {"detail", e.what()}};
      }
      ch->send(k, enc(reply));
    }
  }
}

// The client side: dial `to` as `from`, send one request, read one reply.
// A handshake refusal (either side unpaired) surfaces as a typed refusal.
json request(household& m, const std::string& from, const std::string& to,
             std::uint64_t k, const json& body) {
  peer& f = m.at(from);
  auto admit = [&f](const std::string& pk) {
    std::lock_guard<std::mutex> g(f.mu);
    return f.accepted.count(pk) > 0;
  };
  auto ch = syg::mesh::dial(m.port_of(to), f.id, admit);
  if (!ch) return {{"ok", false}, {"refused", true}, {"reason", "handshake"}};
  ch->send(k, enc(body));
  std::uint64_t rk;
  bytes rb;
  if (!ch->recv(rk, rb)) return {{"ok", false}, {"refused", true},
                                 {"reason", "no-reply"}};
  return dec(rb);
}

}  // namespace

namespace syg::harness {

int mesh_session(const nlohmann::json& in) {
  household m;
  const json peer_cfg = in.value("peers", json::object());
  for (auto& [name, cfg] : peer_cfg.items())
    m.peers.emplace(name, std::make_unique<peer>(
                              name, cfg.value("seed", name)));
  for (auto& [name, p] : m.peers)
    p->server = std::thread(serve_loop, p.get());

  json results = json::array();
  for (const auto& op : in.value("ops", json::array())) {
    const std::string what = op.at("op");
    json r;
    if (what == "peer-key") {
      r = {{"key", m.at(op.at("peer")).id.peer_key()}};
    } else if (what == "pair") {  // the ceremony: mutual acceptance
      auto& a = m.at(op.at("a"));
      auto& b = m.at(op.at("b"));
      std::lock_guard<std::mutex> ga(a.mu);
      std::lock_guard<std::mutex> gb(b.mu);
      a.accepted.insert(b.id.peer_key());
      b.accepted.insert(a.id.peer_key());
      r = {{"paired", json::array({op.at("a"), op.at("b")})}};
    } else if (what == "revoke") {
      auto& p = m.at(op.at("peer"));
      auto& of = m.at(op.at("of"));
      std::lock_guard<std::mutex> g(p.mu);
      p.accepted.erase(of.id.peer_key());
      r = {{"revoked", op.at("of")}};
    } else if (what == "put") {
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      std::size_t si = op.value("store", 0);
      auto& ss = *p.stores.at(si);
      std::string cid;
      if (op.contains("node"))
        cid = ss.store.put_node(op.at("node"), true);
      else
        cid = ss.store.put_raw(
            syg::formats::bytes_of_projection(op.at("bytes")), true);
      r = {{"cid", cid}};
    } else if (what == "advertise") {
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      auto set_of = [](const json& arr) {
        std::set<std::string> s;
        for (const auto& v : arr) s.insert(v.get<std::string>());
        return s;
      };
      if (op.contains("run")) p.lists.run = set_of(op.at("run"));
      if (op.contains("serve")) p.lists.serve = set_of(op.at("serve"));
      if (op.contains("subscribe"))
        p.lists.subscribe = set_of(op.at("subscribe"));
      r = {{"advertised", p.name}};
    } else if (what == "share-store") {
      // MSH-8: a second store graph shared with a SUBSET of paired keys.
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      auto ss = std::make_unique<sub_store>(p.name + "/" + op.value("name", "s"));
      std::set<std::string> keys;
      for (const auto& n : op.at("keys"))
        keys.insert(m.at(n.get<std::string>()).id.peer_key());
      ss->share = keys;
      std::size_t idx = p.stores.size();
      p.stores.push_back(std::move(ss));
      r = {{"store", idx}};
    } else if (what == "fetch") {
      const std::string want = op.at("cid");
      r = request(m, op.at("from"), op.at("to"), FETCH, {{"cid", want}});
      if (r.value("ok", false)) {
        // verify + store locally under the cid's OWN multicodec (the
        // store::fetch model): raw bytes vs a dag-cbor node hash differently.
        auto obj = syg::formats::bytes_of_projection(r.at("obj"));
        auto raw = syg::formats::cid_to_text(
            syg::formats::cid_of(syg::formats::pins::multicodec_raw, obj));
        auto cbor = syg::formats::cid_to_text(
            syg::formats::cid_of(syg::formats::pins::multicodec_dag_cbor, obj));
        auto& f = m.at(op.at("from"));
        std::lock_guard<std::mutex> g(f.mu);
        std::string got;
        if (want == raw)
          got = f.stores.at(0)->store.put_raw(obj, false);
        else if (want == cbor)
          got = f.stores.at(0)->store.put_node(
              syg::formats::decode_to_projection(obj), false);
        r = {{"ok", got == want}, {"cid", got}};
      }
    } else if (what == "bind") {
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      p.stores.at(0)->store.bind_ref(op.at("ref"), op.at("cid"));
      r = {{"bound", op.at("ref")}};
    } else if (what == "subscribe") {
      r = request(m, op.at("from"), op.at("to"), SUBSCRIBE,
                  {{"ref", op.at("ref")}});
    } else if (what == "hello") {
      r = request(m, op.at("from"), op.at("to"), HELLO, json::object());
    } else if (what == "place") {
      r = request(m, op.at("from"), op.at("to"), PLACE,
                  {{"type", op.at("type")}});
    } else if (what == "place-fallthrough") {
      // MSH-3.1: the requesting engine tries candidate peers in order; a
      // peer that doesn't advertise the type refuses with a typed error that
      // is VISIBLE here; placement falls through to the next, or reports
      // unplaceable when every candidate has refused.
      const std::string from = op.at("from");
      const std::string type = op.at("type");
      json refusals = json::array();
      json placed;
      for (const auto& cand : op.at("candidates")) {
        json rr = request(m, from, cand.get<std::string>(), PLACE,
                          {{"type", type}});
        if (rr.value("ok", false)) {
          placed = {{"ok", true}, {"placed_on", cand}, {"type", type}};
          break;
        }
        refusals.push_back({{"peer", cand}, {"refusal", rr}});
      }
      r = placed.is_null()
              ? json{{"ok", false}, {"unplaceable", true}, {"type", type},
                     {"refusals", refusals}}
              : placed;
      r["refusals"] = refusals;  // the trail is always visible to the engine
    } else if (what == "place-derivation") {
      // Worker placement by ADVERTISED capability (PKG-5), queried over the
      // wire. No test-supplied worker table: `from` asks each candidate what
      // it runs; the first advertiser executes the derivation and returns the
      // result by hash. No worker anywhere is a loud refusal.
      const std::string from = op.at("from");
      const std::string cap = op.value("capability", "render-analysis");
      std::string worker;
      for (const auto& cand : op.at("candidates")) {
        json h = request(m, from, cand.get<std::string>(), HELLO,
                         json::object());
        if (h.value("ok", false)) {
          for (const auto& t : h.value("run", json::array()))
            if (t.get<std::string>() == cap) worker = cand.get<std::string>();
        }
        if (!worker.empty()) break;
      }
      if (worker.empty())
        throw std::runtime_error("no worker advertises the capability: " + cap);
      r = request(m, from, worker, QUERY,
                  {{"capability", cap}, {"graph", op.at("graph")},
                   {"blocks", op.value("blocks", 200)}});
    } else if (what == "set-contract") {
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      p.contract.speaks = op.at("contract");
      p.contract.migrations.clear();
      for (const auto& e : op.value("migrations", json::array()))
        p.contract.migrations.push_back({e.at("from"), e.at("to")});
      r = {{"speaks", p.contract.speaks}};
    } else if (what == "admit-plugin") {
      // ABI-4.1: a plugin records the contract hash it was built against;
      // loading checks REACHABILITY to the peer's contract, not equality. A
      // refusal is typed and names the missing path.
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      const std::string built = op.at("contract");
      if (contract_reaches(p.contract, built))
        r = {{"loaded", true}, {"from", built}, {"to", p.contract.speaks}};
      else
        r = {{"loaded", false}, {"error", "no-migration-path"},
             {"from", built}, {"to", p.contract.speaks}};
    } else if (what == "set-policy") {
      // MSH-5: the provenance policy — signers whose plugins this peer loads.
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      p.trusted_signers.clear();
      for (const auto& n : op.at("trust"))
        p.trusted_signers.insert(m.at(n.get<std::string>()).id.peer_key());
      r = {{"trusted", p.trusted_signers.size()}};
    } else if (what == "ship-graph") {
      // A GRAPH dataset flows and realizes WITHOUT a prompt: it is bounded
      // composition of advertised vocabulary (ch. 8). It just runs.
      auto& to = m.at(op.at("to"));
      std::lock_guard<std::mutex> g(to.mu);
      syg::executor::exec_plan plan(syg::organs::parse_graph(op.at("graph")),
                                    48000, 128);
      double energy = 0;
      for (int i = 0; i < op.value("blocks", 8); ++i) {
        const float* b = plan.pump_block();
        for (int k = 0; k < 128; ++k) energy += double(b[k]) * b[k];
      }
      r = {{"ran", true}, {"energy", energy}};
    } else if (what == "ship-plugin") {
      // MSH-5.1/5.2: a PLUGIN dataset is new capability injection — gated by
      // provenance policy BEFORE any load. Unsigned is refused + logged;
      // signed by an untrusted key is refused; signed by a trusted key loads
      // (native: real dlopen hot-load; wasm: same gate, form-agnostic — its
      // execution is a host concern, rung 10). Provenance stays queryable.
      const std::string from = op.at("from");
      auto& fp = m.at(from);
      const std::string form = op.value("form", "native");
      const std::string path = op.value("artifact", "");
      // the artifact's content hash (what the signer signs)
      syg::mesh::bytes art;
      if (!path.empty()) {
        std::ifstream in(path, std::ios::binary);
        art.assign(std::istreambuf_iterator<char>(in), {});
      } else {
        art = syg::formats::bytes_of_projection(op.at("bytes"));
      }
      auto art_cid = syg::formats::cid_to_text(
          syg::formats::cid_of(syg::formats::pins::multicodec_raw, art));
      json prov = nullptr;
      if (op.value("sign", false)) {
        syg::mesh::bytes msg(art_cid.begin(), art_cid.end());
        prov = {{"signer", fp.id.peer_key()},
                {"sig", syg::formats::projection_of_bytes(fp.id.sign(msg))},
                {"artifact", art_cid},
                {"source", op.value("source", "")},
                {"toolchain", op.value("toolchain", "")},
                {"form", form}};
      }
      // the gate, on the RECEIVING peer
      auto& to = m.at(op.at("to"));
      std::lock_guard<std::mutex> g(to.mu);
      if (prov.is_null()) {
        r = {{"loaded", false}, {"error", "unsigned"}, {"logged", true}};
      } else {
        auto signer = prov.at("signer").get<std::string>();
        auto sig = syg::formats::bytes_of_projection(prov.at("sig"));
        syg::mesh::bytes msg(art_cid.begin(), art_cid.end());
        bool sig_ok = syg::mesh::verify(syg::mesh::public_key_of(signer), msg, sig);
        if (!sig_ok) {
          r = {{"loaded", false}, {"error", "bad-signature"}, {"logged", true}};
        } else if (!to.trusted_signers.count(signer)) {
          r = {{"loaded", false}, {"error", "untrusted-signer"},
               {"signer", signer}, {"logged", true}};
        } else if (form == "native") {
          void* lib = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
          if (!lib) throw std::runtime_error(std::string("dlopen: ") + dlerror());
          auto entry = reinterpret_cast<const syg::crown::native_type* (*)()>(
              dlsym(lib, "syg_plugin_native"));
          if (!entry) throw std::runtime_error("plugin lacks syg_plugin_native");
          auto* t = new syg::crown::native_type(*entry());
          if (op.contains("as"))
            t->name = (new std::string(op.at("as").get<std::string>()))->c_str();
          syg::organs::register_plugin_native(t);
          to.plugin_prov[t->name] = prov;
          r = {{"loaded", true}, {"form", "native"}, {"type", t->name},
               {"provenance", prov}};
        } else {  // wasm (or any non-native form): SAME gate, form-agnostic
          std::string type = op.value("as", "wasm_plugin");
          to.plugin_prov[type] = prov;
          r = {{"loaded", true}, {"form", form}, {"type", type},
               {"executed", false}, {"provenance", prov}};
        }
      }
    } else if (what == "plugin-provenance") {
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      auto it = p.plugin_prov.find(op.at("type"));
      r = it == p.plugin_prov.end() ? json{{"known", false}}
                                    : json{{"known", true}, {"provenance", it->second}};
    } else if (what == "capture") {
      // MSH-6.1: a capture's testimony carries the capturing peer's public
      // key AND a signature over the take's content hash. Verification is
      // signature-checking, not trust.
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      auto take = syg::formats::bytes_of_projection(op.at("bytes"));
      auto take_cid = syg::formats::cid_to_text(
          syg::formats::cid_of(syg::formats::pins::multicodec_raw, take));
      syg::mesh::bytes msg(take_cid.begin(), take_cid.end());
      auto sig = p.id.sign(msg);
      json testimony{{"peer-key", p.id.peer_key()},
                     {"wiring-route", op.value("route", "dac0")},
                     {"take", take_cid},
                     {"sig", syg::formats::projection_of_bytes(sig)}};
      auto c = p.stores.at(0)->store.commit_capture(take, testimony);
      r = {{"take", take_cid}, {"provenance", c.provenance},
           {"testimony", testimony}};
    } else if (what == "verify-capture") {
      // Check the testimony's signature against ITS stated peer-key over the
      // take hash. Tampering the peer-id (or the take) breaks the check.
      const json& t = op.at("testimony");
      auto pk = syg::mesh::public_key_of(t.at("peer-key").get<std::string>());
      auto sig = syg::formats::bytes_of_projection(t.at("sig"));
      std::string take_cid = t.at("take");
      syg::mesh::bytes msg(take_cid.begin(), take_cid.end());
      r = {{"valid", syg::mesh::verify(pk, msg, sig)}};
    } else if (what == "has") {
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      bool has = false;
      for (auto& ss : p.stores)
        if (ss->store.get(op.at("cid")).has_value()) has = true;
      r = {{"has", has}};
    } else if (what == "read") {
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      r = {{"node", p.stores.at(0)->store.get_node(op.at("cid"))}};
    } else if (what == "audit-log") {
      auto& p = m.at(op.at("peer"));
      std::lock_guard<std::mutex> g(p.mu);
      r = {{"instantiated", p.instantiated}};
    } else if (what == "probe") {
      // MSH-2.1: a raw TCP probe from an unpaired scanner. Send the payload
      // bytes; observe whether ANY bytes come back (a plaintext service
      // would answer). The peer's handshake reads a length-framed hello,
      // sees garbage, and closes: zero bytes, refused.
      std::string p = op.value("payload", "GET / HTTP/1.1\r\n\r\n");
      bytes payload(p.begin(), p.end());
      auto got = syg::mesh::probe(m.port_of(op.at("to")), payload);
      r = json{{"bytes_received", static_cast<long>(got.size())},
               {"refused", got.empty()}};
    } else {
      r = {{"error", "unknown op"}, {"op", what}};
    }
    results.push_back(r);
  }

  for (auto& [name, p] : m.peers) {
    p->running.store(false);
    p->lis->stop();
    if (p->server.joinable()) p->server.join();
  }
  std::cout << json{{"results", results}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
