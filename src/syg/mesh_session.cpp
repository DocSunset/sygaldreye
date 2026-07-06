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

#include "dagcbor/dagcbor.hpp"
#include "identity/identity.hpp"
#include "link/link.hpp"
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

struct peer {
  std::string name;
  syg::mesh::identity id;
  std::vector<std::unique_ptr<sub_store>> stores;  // [0] = default (all paired)
  advert lists;
  std::set<std::string> accepted;               // paired peer-keys
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
      const std::string cid = body.at("cid");
      for (auto& ss : me.stores) {
        if (!ss->store.provides(cid)) continue;
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
      json reply = handle(*me, k, dec(reqb), ch->remote_peer_key());
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
      r = request(m, op.at("from"), op.at("to"), FETCH,
                  {{"cid", op.at("cid")}});
      if (r.value("ok", false)) {
        // verify + store locally: the fetched bytes must hash to the cid.
        auto obj = syg::formats::bytes_of_projection(r.at("obj"));
        auto& f = m.at(op.at("from"));
        std::lock_guard<std::mutex> g(f.mu);
        std::string got = f.stores.at(0)->store.put_raw(obj, false);
        r = {{"ok", got == op.at("cid").get<std::string>()},
             {"cid", got}};
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
