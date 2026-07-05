#include "store_session.hpp"

#include <iostream>
#include <map>
#include <stdexcept>

#include "chunk/chunk.hpp"
#include "dagcbor/dagcbor.hpp"
#include "exec_plan.hpp"
#include "parser/parser.hpp"
#include "query/query.hpp"
#include "registry_face/registry_face.hpp"
#include "store.hpp"

namespace syg::harness {
namespace {

using store::peer_store;

// commit a composite graph: topology / defaults / lock as separate objects,
// the graph node linking them (STO-7); orphaned defaults surface as
// conflicts, never silence (STO-7.2)
nlohmann::json commit_graph(peer_store& s, const nlohmann::json& interchange,
                            const std::string& refname) {
  auto doc = organs::parse_graph(interchange);
  nlohmann::json conflicts = nlohmann::json::array();
  std::set<std::string> ids;
  for (const auto& [id, t] : doc.nodes) ids.insert(id);
  for (const auto& [route, v] : doc.defaults.items())
    if (!ids.count(route.substr(0, route.find('/'))))
      conflicts.push_back("orphaned default: " + route);
  nlohmann::ordered_json topo, nodes;
  for (const auto& [id, t] : doc.nodes) nodes[id] = {{"type", t}};
  topo["nodes"] = nodes;
  auto edges = nlohmann::json::array();
  for (const auto& [f, t] : doc.edges) edges.push_back({{"from", f}, {"to", t}});
  topo["edges"] = edges;
  auto topology_cid = s.put_node(topo, false);
  auto defaults_cid = s.put_node(doc.defaults, false);
  auto lock_cid = s.put_node(doc.lock, false);
  auto graph_cid = s.put_node({{"kind", "graph"},
                               {"topology", {{"/", topology_cid}}},
                               {"defaults", {{"/", defaults_cid}}},
                               {"lock", {{"/", lock_cid}}}},
                              false);
  if (!refname.empty() && conflicts.empty()) s.bind_ref(refname, graph_cid);
  return {{"graph", graph_cid},
          {"topology", topology_cid},
          {"defaults", defaults_cid},
          {"conflicts", conflicts}};
}

nlohmann::json backlink_query(const peer_store& s, const std::string& cid,
                              long* evals) {
  // LNG-10.4: the back-link lookup IS a query — one evaluator, no bespoke path
  organs::graph_doc q;
  q.kind = "graph";
  q.nodes = {{"s0", "seed"}, {"t0", "traverse"}};
  q.edges = {{"s0/out", "t0/in"}};
  q.defaults = {{"s0/cids", {cid}}, {"t0/via", "backlinks"}};
  auto r = organs::eval_query(q, s, evals);
  return nlohmann::json(std::vector<std::string>(r.begin(), r.end()));
}

}  // namespace

int store_session(const nlohmann::json& in) {
  std::map<std::string, peer_store> peers;
  const auto peer_cfg = in.value("peers", nlohmann::json::object());
  for (const auto& [name, cfg] : peer_cfg.items())
    peers.emplace(name, peer_store(name, cfg.value("provide_all", false)));
  if (peers.empty()) peers.emplace("local", peer_store("local"));

  // query memo (LNG-10.1): a run is a committed derivation over the store
  std::map<std::string, nlohmann::json> query_memo;
  long commit_epoch = 0;
  // the standing query (LNG-10.2): result + delta maintenance
  organs::graph_doc standing;
  std::set<std::string> standing_result;
  bool has_standing = false;
  long standing_evals = 0;

  auto results = nlohmann::json::array();
  for (const auto& op : in.value("ops", nlohmann::json::array())) {
    const std::string what = op.at("op");
    auto& s = peers.at(op.value("peer", peers.begin()->first));
    nlohmann::json r;
    if (what == "put") {
      auto bytes = formats::bytes_of_projection(op.at("bytes"));
      r = {{"cid", s.put_raw(bytes, op.value("provide", false))}};
    } else if (what == "commit-backlink-index") {
      // STO-9.1: the index is itself derived data — deterministic, so
      // deleting and re-deriving hashes identically
      nlohmann::ordered_json index;
      for (const auto& [cid, obj] : s.objects()) {
        const auto& b = s.backlinks(cid);
        if (!b.empty()) index[cid] = b;
      }
      r = {{"index", s.put_node(index, false)}};
    } else if (what == "bind-ref") {
      s.bind_ref(op.at("ref"), op.at("cid"));
      r = nullptr;
    } else if (what == "put-node") {
      r = {{"cid", s.put_node(op.at("node"), op.value("provide", false))}};
    } else if (what == "get") {
      auto bytes = s.get(op.at("cid"));
      r = bytes ? nlohmann::json{{"hit", true}, {"size", bytes->size()}}
                : nlohmann::json{{"hit", false}};  // the clean miss
    } else if (what == "put-blob") {
      // a take-sized blob (chunked); deterministic bytes from a seed
      long n = op.at("size");
      formats::byte_vec blob(static_cast<std::size_t>(n));
      unsigned lcg = op.value("seed", 7);
      for (auto& b : blob) b = static_cast<std::uint8_t>((lcg = lcg * 1103515245 + 12345) >> 24);
      auto root = formats::chunk_put(blob, [&](const formats::byte_vec& cid,
                                               const formats::byte_vec& obj) {
        (void)cid;
        try {
          s.put_node(formats::decode_to_projection(obj), op.value("provide", false));
        } catch (const std::exception&) {
          s.put_raw(obj, op.value("provide", false));
        }
      });
      r = {{"cid", formats::cid_to_text(root)}};
    } else if (what == "commit-graph") {
      r = commit_graph(s, op.at("graph"), op.value("ref", ""));
    } else if (what == "read") {
      // the store-graph face: get = wiring — walk ref -> graph -> member
      const auto* cid = s.ref(op.at("ref"));
      if (!cid) throw std::runtime_error("no such ref");
      auto node = s.get_node(*cid);
      auto member = op.value("member", "");
      if (!member.empty())
        node = s.get_node(node.at(member).at("/").get<std::string>());
      r = {{"node", node}};
    } else if (what == "compile") {
      // STO-3.1: compiling is a committed derivation, memoized
      const auto* app = s.ref(op.at("ref"));
      if (!app) throw std::runtime_error("no such ref");
      auto engine_cid = s.put_node({{"kind", "engine"}, {"name", "engine-v0"}}, false);
      nlohmann::json recipe{{"op", "compile"},
                            {"inputs", {{"app", {{"/", *app}}},
                                        {"engine", {{"/", engine_cid}}}}},
                            {"determinism", "exact"}};
      if (auto hit = s.memo_lookup(recipe)) {
        r = {{"execution", hit->output}, {"provenance", hit->provenance},
             {"memo", true}, {"passes_run", 0}};
      } else {
        auto graph = s.get_node(*app);
        auto topo = s.get_node(graph.at("topology").at("/").get<std::string>());
        nlohmann::json execution{{"kind", "execution-graph"},
                                 {"nodes", topo.at("nodes")},
                                 {"engine", {{"/", engine_cid}}}};
        auto c = s.commit_derivation(recipe, execution);
        r = {{"execution", c.output}, {"provenance", c.provenance},
             {"memo", false}, {"passes_run", 1}};
      }
    } else if (what == "record") {
      // STO-3.2: a capture through a recorder; testimony carries the peer
      // key and the wiring route
      syg::executor::exec_plan p(organs::parse_graph(op.at("graph")), 48000, 128);
      formats::byte_vec take;
      int blocks = static_cast<int>(op.value("seconds", 2.0) * 48000) / 128;
      for (int i = 0; i < blocks; ++i) {
        const float* out = p.pump_block();
        auto* raw = reinterpret_cast<const std::uint8_t*>(out);
        take.insert(take.end(), raw, raw + 128 * sizeof(float));
      }
      auto c = s.commit_capture(take, {{"peer-key", "z6Mk" + s.name()},
                                       {"wiring", "nodes/dac0/in"},
                                       {"kind", "take"}});
      r = {{"take", c.output}, {"testimony", c.provenance}};
    } else if (what == "commit-stream") {
      r = {{"refused",
            "a stream cannot be committed: wire a recorder node and commit "
            "its capture (L5)"}};
    } else if (what == "edit-commit") {
      // freq 220 -> 330 shaped edit: new defaults object, new graph, rebind
      const auto* cid = s.ref(op.at("ref"));
      auto graph = s.get_node(*cid);
      auto defaults = s.get_node(graph.at("defaults").at("/").get<std::string>());
      defaults[op.at("route").get<std::string>()] = op.at("value");
      auto new_defaults = s.put_node(defaults, false);
      graph["defaults"] = {{"/", new_defaults}};
      auto new_graph = s.put_node(graph, false);
      s.bind_ref(op.at("ref"), new_graph);
      r = {{"graph", new_graph}};
    } else if (what == "undo-ref") {
      r = {{"undone", s.undo_ref(op.at("ref"))},
           {"now", s.ref(op.at("ref")) ? *s.ref(op.at("ref")) : ""}};
    } else if (what == "trail") {
      r = {{"trail", s.trail(op.at("ref"))}};
    } else if (what == "evict") {
      s.evict_unprovided();
      r = {{"objects", s.objects().size()}};
    } else if (what == "fetch") {
      auto& src = peers.at(op.at("from").get<std::string>());
      r = {{"moved", store::fetch(s, src, op.at("cid"),
                                  op.value("stop_after", -1L))}};
    } else if (what == "provided-by") {
      auto who = nlohmann::json::array();
      for (auto& [name, p] : peers)
        if (p.provides(op.at("cid"))) who.push_back(name);
      r = {{"provided_by", who}};
    } else if (what == "unprovide-everywhere") {
      for (auto& [name, p] : peers) p.unprovide(op.at("cid"));
      r = nullptr;
    } else if (what == "has") {
      r = {{"has", s.get(op.at("cid")).has_value()}};
    } else if (what == "backlinks") {
      long evals = 0;
      r = {{"backlinks", backlink_query(s, op.at("cid"), &evals)},
           {"evals", evals}};
    } else if (what == "query") {
      auto qdoc = organs::parse_graph(op.at("query"));
      auto qcid = s.put_node(op.at("query"), false);
      auto key = qcid + ":" + std::to_string(commit_epoch);
      if (auto it = query_memo.find(key); it != query_memo.end()) {
        r = it->second;
        r["memo"] = true;
      } else {
        long evals = 0;
        auto set = organs::eval_query(qdoc, s, &evals);
        r = {{"result", std::vector<std::string>(set.begin(), set.end())},
             {"evals", evals}, {"memo", false}};
        query_memo[key] = r;
      }
    } else if (what == "standing-query") {
      standing = organs::parse_graph(op.at("query"));
      standing_result = organs::eval_query(standing, s, &standing_evals);
      has_standing = true;
      r = {{"result", std::vector<std::string>(standing_result.begin(),
                                               standing_result.end())},
           {"evals", standing_evals}};
    } else if (what == "commit-take-chained") {
      // a new qualifying commit: chained to an existing provenance output
      nlohmann::json recipe{{"op", "render"},
                            {"inputs", {{"src", {{"/", op.at("parent").get<std::string>()}}}}},
                            {"n", op.value("n", 0)},
                            {"kind", "take"},
                            {"determinism", "exact"}};
      auto c = s.commit_derivation(recipe, {{"kind", "take"}, {"n", op.value("n", 0)}});
      r = {{"take", c.output}, {"provenance", c.provenance}};
      if (has_standing) {
        // the standing form: the new commit seeds a DELTA pass (LNG-10.2)
        long evals = 0;
        std::set<std::string> delta{c.provenance, c.output};
        auto grown = organs::eval_query(standing, s, &evals, &delta);
        standing_result.insert(grown.begin(), grown.end());
        r["standing_evals"] = evals;
        r["standing_size"] = standing_result.size();
      }
    } else if (what == "standing-result") {
      r = {{"result", std::vector<std::string>(standing_result.begin(),
                                               standing_result.end())}};
    } else if (what == "palette-query") {
      // LNG-10.4: the palette is a query over the committed registry
      for (const auto* n : organs::registered_natives())
        s.put_node({{"kind", "node-type"}, {"name", n->name}}, false);
      organs::graph_doc q;
      q.kind = "graph";
      q.nodes = {{"s0", "seed"}, {"f0", "filter"}};
      q.edges = {{"s0/out", "f0/in"}};
      q.defaults = {{"s0/all", true}, {"f0/key", "kind"},
                    {"f0/equals", "node-type"}};
      long evals = 0;
      auto set = organs::eval_query(q, s, &evals);
      auto names = nlohmann::json::array();
      for (const auto& c : set) names.push_back(s.get_node(c)["name"]);
      r = {{"palette", names}};
    } else {
      throw std::runtime_error("unknown store op: " + what);
    }
    if (what.starts_with("put") || what.starts_with("commit") ||
        what == "record" || what == "edit-commit")
      ++commit_epoch;
    results.push_back(r);
  }
  std::cout << nlohmann::json{{"results", results}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
