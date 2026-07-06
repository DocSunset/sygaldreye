#include "conform_session.hpp"

#include <dlfcn.h>

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "store.hpp"

namespace syg::harness {
namespace {

using nlohmann::json;
using store::peer_store;

// Walk a succession node back to its origin, returning the chain origin->node
// (each entry is the node's committed record).
std::vector<json> chain_to_origin(const peer_store& s, const std::string& cid) {
  std::vector<json> back;
  std::string cur = cid;
  while (!cur.empty()) {
    auto node = s.get_node(cur);
    back.push_back(node);
    cur = node.contains("supersedes") && !node["supersedes"].is_null()
              ? node["supersedes"]["/"].get<std::string>()
              : std::string();
  }
  std::vector<json> fwd(back.rbegin(), back.rend());
  return fwd;
}

// Derived version (ADR-032): M breaking hops from origin, m additive since the
// last breaking, p fix since the last additive. Numbers are stored NOWHERE —
// computed from the chain, so they can never disagree with history.
std::string derive_version(const std::vector<json>& chain) {
  int M = 0, m = 0, p = 0;
  for (const auto& node : chain) {
    const std::string c = node.value("class", "origin");
    if (c == "breaking") { ++M; m = 0; p = 0; }
    else if (c == "additive") { ++m; p = 0; }
    else if (c == "fix") { ++p; }
    // "origin" contributes nothing
  }
  return std::to_string(M) + "." + std::to_string(m) + "." + std::to_string(p);
}

// The green criteria a chain has accumulated (a criterion stays green unless a
// later succession regresses it) — the predecessor baseline the class gate
// checks additive/fix successions against.
std::map<std::string, bool> accumulated_criteria(const std::vector<json>& chain) {
  std::map<std::string, bool> acc;
  for (const auto& node : chain) {
    const json crit = node.value("criteria", json::object());
    for (auto it = crit.begin(); it != crit.end(); ++it)
      acc[it.key()] = it.value().get<bool>();
  }
  return acc;
}

}  // namespace

int conform_session(const nlohmann::json& in) {
  peer_store s("conform");
  std::map<std::string, std::string> heads;  // name -> latest cid in its chain
  json results = json::array();

  for (const auto& op : in.value("ops", json::array())) {
    const std::string what = op.at("op");
    json r;
    if (what == "succeed") {
      // declare a succession: {name, of?, class, criteria?}. Classes are
      // VERIFIED, not vowed (ADR-032): an additive/fix succession that
      // regresses a predecessor's green criterion is REJECTED, naming it.
      const std::string name = op.at("name"), cls = op.value("class", "origin");
      json node{{"kind", "succession"}, {"name", name}, {"class", cls},
                {"criteria", op.value("criteria", json::object())}};
      std::string of;
      if (op.contains("of")) {
        of = heads.at(op.at("of").get<std::string>());  // supersede its head
        node["supersedes"] = {{"/", of}};
      }
      if ((cls == "additive" || cls == "fix") && !of.empty()) {
        auto base = accumulated_criteria(chain_to_origin(s, of));
        const auto& here = node["criteria"];
        for (const auto& [id, was_green] : base)
          if (was_green && here.contains(id) && !here[id].get<bool>())
            throw std::runtime_error(
                "class '" + cls + "' succession regresses a green predecessor "
                "criterion: " + id);
      }
      auto cid = s.put_node(node, false);
      heads[name] = cid;
      r = {{"cid", cid}, {"version", derive_version(chain_to_origin(s, cid))}};
    } else if (what == "version") {
      std::string cid = op.contains("cid") ? op.at("cid").get<std::string>()
                                           : heads.at(op.at("name"));
      r = {{"version", derive_version(chain_to_origin(s, cid))}};
    } else if (what == "migrate-read") {
      // CNF-4: an old object is readable via LAZY migrate-on-read — the
      // migration is a memoized derivation (ADR-025), so the second read of
      // the same object is a memo hit (no re-run).
      auto obj_cid = s.put_node(op.at("object"), false);
      json recipe{{"op", "migrate"},
                  {"inputs", {{"obj", {{"/", obj_cid}}}}},
                  {"from", op.at("from")}, {"to", op.at("to")},
                  {"determinism", "exact"}};
      bool memo = s.memo_lookup(recipe).has_value();
      // the migrated form: the same data, retagged to the new kind
      json migrated = op.at("object");
      migrated["kind"] = op.at("to");
      auto c = s.commit_derivation(recipe, migrated);
      r = {{"output", c.output}, {"memo", memo || c.memo_hit},
           {"migrated", s.get_node(c.output)}};
    } else if (what == "mixed-exchange") {
      // CNF-4: a mixed-version two-peer mesh exchanges BOTH directions. Peer A
      // speaks kind-v1, peer B kind-v2. A's v1 object flows to B and migrates
      // on read to v2; B's v2 object flows to A and migrates to v1. The
      // migration is a dataset either peer can run (ADR-025).
      peer_store a("A"), b("B");
      auto migrate = [](peer_store& dst, const json& obj, const std::string& to) {
        json recipe{{"op", "migrate"},
                    {"inputs", {{"obj", {{"/", dst.put_node(obj, false)}}}}},
                    {"to", to}, {"determinism", "exact"}};
        json m = obj;
        m["kind"] = to;
        return dst.get_node(dst.commit_derivation(recipe, m).output);
      };
      // A -> B: a v1 object read as v2 on the far peer
      json v1{{"kind", "kind-v1"}, {"payload", op.value("a_payload", 1)}};
      auto v1_cid = a.put_node(v1, true);
      store::fetch(b, a, v1_cid);
      auto at_b = migrate(b, b.get_node(v1_cid), "kind-v2");
      // B -> A: a v2 object read as v1 on the near peer
      json v2{{"kind", "kind-v2"}, {"payload", op.value("b_payload", 2)}};
      auto v2_cid = b.put_node(v2, true);
      store::fetch(a, b, v2_cid);
      auto at_a = migrate(a, a.get_node(v2_cid), "kind-v1");
      r = {{"a_to_b", at_b}, {"b_to_a", at_a},
           {"both_directions", at_b.at("kind") == "kind-v2" &&
                                   at_a.at("kind") == "kind-v1"}};
    } else if (what == "lock-swap") {
      // CNF-4: a vocabulary upgrade is ONE lock swap (ADR-026) — the lock maps
      // names to type CIDs; the TOPOLOGY object (and thus its hash and
      // provenance chain) is untouched. The graph references its topology by
      // hash, so swapping the lock mints a new graph over the SAME topology.
      auto topo_cid = s.put_node(op.at("topology"), false);
      const std::string tname = op.value("type", "osc");
      // the two vocabulary versions: name -> a v1 type CID, then a v2 type CID
      auto tv1 = s.put_node({{"kind", "node-type"}, {"name", tname}, {"v", 1}}, false);
      auto tv2 = s.put_node({{"kind", "node-type"}, {"name", tname}, {"v", 2}}, false);
      json g_old{{"kind", "graph"}, {"topology", {{"/", topo_cid}}},
                 {"lock", {{tname, {{"/", tv1}}}}}};
      json g_new{{"kind", "graph"}, {"topology", {{"/", topo_cid}}},
                 {"lock", {{tname, {{"/", tv2}}}}}};
      auto old_cid = s.put_node(g_old, false);
      auto new_cid = s.put_node(g_new, false);
      // the topology hash reached from each graph is identical (same link)
      auto topo_of = [&](const std::string& gcid) {
        return s.get_node(gcid).at("topology").at("/").get<std::string>();
      };
      r = {{"graph_old", old_cid}, {"graph_new", new_cid},
           {"topology_cid", topo_cid},
           {"topology_unchanged", topo_of(old_cid) == topo_of(new_cid) &&
                                      topo_of(new_cid) == topo_cid},
           {"graph_changed", old_cid != new_cid}};
    } else if (what == "two-profiles") {
      // CNF-3 / COR-4: a CROWNLESS build (a sealed frozen movement — escapement
      // + baked math, no crown, no plan) passes MOVEMENT-level conformance
      // (it renders) but FAILS PEER-level: it exposes no wire surface, and
      // that absence is DETECTED, not excused (TCF-5).
      void* lib = dlopen(op.at("so").get<std::string>().c_str(),
                         RTLD_NOW | RTLD_LOCAL);
      if (!lib) throw std::runtime_error(std::string("dlopen: ") + dlerror());
      auto create = reinterpret_cast<void* (*)()>(dlsym(lib, "frozen_create"));
      auto pump = reinterpret_cast<void (*)(void*, float*)>(dlsym(lib, "frozen_pump"));
      auto destroy = reinterpret_cast<void (*)(void*)>(dlsym(lib, "frozen_destroy"));
      // the movement profile: the frozen entry points exist and render sound
      json movement;
      if (create && pump && destroy) {
        void* inst = create();
        std::vector<float> buf(128);
        double energy = 0;
        for (int i = 0; i < 200; ++i) {
          pump(inst, buf.data());
          for (float x : buf) energy += double(x) * x;
        }
        destroy(inst);
        movement = {{"pass", energy > 0.0}, {"energy", energy}};
      } else {
        movement = {{"pass", false}, {"reason", "no frozen movement entry points"}};
      }
      // the peer profile: a crownless movement exposes NO wire surface — no
      // peer boot symbol. The absence is detected (the profile FAILS here).
      bool has_peer = dlsym(lib, "syg_peer_boot") != nullptr;
      json peer{{"pass", has_peer}};
      if (!has_peer)
        peer["reason"] = "crownless build exposes no peer/wire surface";
      r = {{"movement", movement}, {"peer", peer}};
    } else if (what == "resolve") {
      // ref-sugar `name@M.m.p` -> the hash reached by walking the chain to the
      // node whose derived version matches (CNF-6.3).
      const std::string name = op.at("name"), at = op.at("at");
      std::string found;
      auto chain = chain_to_origin(s, heads.at(name));
      for (std::size_t i = 0; i < chain.size(); ++i) {
        std::vector<json> prefix(chain.begin(), chain.begin() + i + 1);
        if (derive_version(prefix) == at)
          found = s.put_node(chain[i], false);  // deterministic: same bytes->same cid
      }
      if (found.empty())
        throw std::runtime_error("no node at " + name + "@" + at);
      r = {{"cid", found}, {"resolved", name + "@" + at}};
    } else {
      r = {{"error", "unknown op"}, {"op", what}};
    }
    results.push_back(r);
  }
  std::cout << json{{"results", results}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
