#include "compile_session.hpp"

#include <fstream>
#include <iostream>
#include <map>

#include "compiler.hpp"
#include "exec_plan.hpp"
#include "parser/parser.hpp"
#include "query/query.hpp"
#include "store.hpp"

namespace syg::harness {
namespace {

organs::graph_doc load_engine(const nlohmann::json& splice) {
  std::ifstream f("vocabulary/engine-v0.json");
  auto doc = organs::parse_graph(nlohmann::json::parse(f));
  if (!splice.is_null()) {  // the well-behaved package: additive wiring only
    for (const auto& [id, rec] : splice.at("add_nodes").items())
      doc.nodes.emplace_back(id, rec.at("type"));
    for (const auto& e : splice.at("add_edges"))
      doc.edges.emplace_back(e.at("from"), e.at("to"));
  }
  return doc;
}

}  // namespace

int compile_session(const nlohmann::json& in) {
  store::peer_store s("local");
  nlohmann::json app;  // the app graph, the ONE definition (L13)
  long engine_instances = 0;
  auto results = nlohmann::json::array();
  for (const auto& op : in.value("ops", nlohmann::json::array())) {
    const std::string what = op.at("op");
    nlohmann::json r;
    if (what == "set-app") {
      app = op.at("graph");
      r = nullptr;
    } else if (what == "compile") {
      auto engine = load_engine(op.value("splice", nlohmann::json()));
      auto c = executor::compile(app, engine, s);
      r = {{"execution", c.execution_cid},
           {"provenance", c.provenance_cid},
           {"map", c.execution["map"]},
           {"mappings", c.execution["mappings"]},
           {"passes_run", c.passes_run},
           {"memo", c.memo},
           {"structural_memo", c.structural_memo},
           {"engine_instances", engine_instances}};
    } else if (what == "engine-diff") {
      // CMP-3.1: spliced vs vanilla — additive wiring only
      auto vanilla = load_engine(nlohmann::json());
      auto spliced = load_engine(op.at("splice"));
      bool additive = true;
      std::set<std::pair<std::string, std::string>> vn(vanilla.nodes.begin(),
                                                       vanilla.nodes.end()),
          sn(spliced.nodes.begin(), spliced.nodes.end());
      std::set<std::pair<std::string, std::string>> ve(vanilla.edges.begin(),
                                                       vanilla.edges.end()),
          se(spliced.edges.begin(), spliced.edges.end());
      for (const auto& n : vn)
        if (!sn.count(n)) additive = false;
      for (const auto& e : ve)
        if (!se.count(e)) additive = false;
      r = {{"additive", additive},
           {"added_nodes", sn.size() - vn.size()},
           {"added_edges", se.size() - ve.size()}};
    } else if (what == "edit-app") {
      // an ordinary app edit (defaults or structure)
      if (op.contains("route")) app["defaults"][op.at("route").get<std::string>()] = op.at("value");
      if (op.contains("add_node"))
        app["topology"]["nodes"][op.at("add_node").get<std::string>()] =
            {{"type", op.at("type")}};
      r = nullptr;
    } else if (what == "edit-execution") {
      // CMP-4: an edit in the realized view writes back through the
      // INVERSE map as app-graph edits; a vanished target is a conflict
      auto engine = load_engine(nlohmann::json());
      auto c = executor::compile(app, engine, s);
      const std::string target = op.at("target");  // an execution route
      std::string app_route;
      for (const auto& [ar, xr] : c.execution["map"].items())
        if (xr == target) app_route = ar;
      if (op.value("kind", "") == "replace-mapping") {
        // replace the derived latch at an edge with a mapping node: the
        // write-back is the app-graph edit sequence
        long edge = op.at("edge");
        auto& edges = app["topology"]["edges"];
        auto from = edges[edge]["from"].get<std::string>();
        auto to = edges[edge]["to"].get<std::string>();
        nlohmann::json kept = nlohmann::json::array();
        for (long i = 0; i < static_cast<long>(edges.size()); ++i)
          if (i != edge) kept.push_back(edges[i]);
        auto with = op.at("with").get<std::string>();
        app["topology"]["nodes"][with + "0"] = {{"type", with}};
        kept.push_back({{"from", from}, {"to", with + "0/in"}});
        kept.push_back({{"from", with + "0/out"}, {"to", to}});
        app["topology"]["edges"] = kept;
        r = {{"written_back", true}};
      } else if (app_route.empty()) {
        r = {{"conflict", "execution route '" + target +
                              "' has no app-side source — the target "
                              "vanished upstream (rebase, not silence)"}};
      } else {
        app["defaults"][app_route.substr(6) + "/" +
                        op.at("param").get<std::string>()] = op.at("value");
        r = {{"written_back", true}, {"app_route", app_route}};
      }
    } else if (what == "fork") {
      // CMP-5: rebinding away from the derivation's output, recorded
      auto engine = load_engine(nlohmann::json());
      auto c = executor::compile(app, engine, s);
      s.bind_ref(op.at("ref"), c.execution_cid);
      auto detach = s.put_node({{"op", "fork"},
                                {"detached_from", {{"/", c.provenance_cid}}},
                                {"ref", op.at("ref").get<std::string>()}},
                               false);
      // index the detachment so lineage queries see it
      s.commit_derivation({{"op", "detachment"},
                           {"inputs", {{"origin", {{"/", c.provenance_cid}}}}},
                           {"determinism", "exact"}},
                          {{"kind", "fork-record"}, {"note", detach}});
      r = {{"fork", c.execution_cid}};
    } else if (what == "ref") {
      const auto* c = s.ref(op.at("ref"));
      r = {{"cid", c ? *c : ""}};
    } else if (what == "lineage") {
      auto set = s.backlinks(op.at("cid"));
      nlohmann::json kinds = nlohmann::json::array();
      for (const auto& p : set) {
        auto node = s.get_node(p);
        kinds.push_back(node.value("op", node.value("kind", "?")));
      }
      r = {{"records", set}, {"ops", kinds}};
    } else if (what == "open-engine-editor") {
      // the tower is lazy: a level's engine instantiates ONLY when edited
      ++engine_instances;
      r = {{"engine_instances", engine_instances}};
    } else if (what == "render-blocks") {
      // steady-state playback: realize the app (interpret backend) and pump
      executor::exec_plan p(organs::parse_graph(app), 48000, 128);
      for (int i = 0; i < op.value("blocks", 10); ++i) p.pump_block();
      r = {{"engine_instances", engine_instances}};
    } else {
      throw std::runtime_error("unknown compile op: " + what);
    }
    results.push_back(r);
  }
  std::cout << nlohmann::json{{"results", results}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
