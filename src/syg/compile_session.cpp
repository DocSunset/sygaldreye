#include "compile_session.hpp"

#include <fstream>
#include <iostream>
#include <set>

#include "exec_plan.hpp"
#include "parser/parser.hpp"
#include "realized_compile.hpp"
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
  auto results = nlohmann::json::array();
  for (const auto& op : in.value("ops", nlohmann::json::array())) {
    const std::string what = op.at("op");
    nlohmann::json r;
    if (what == "set-app") {
      app = op.at("graph");
      r = nullptr;
    } else if (what == "compile") {
      // the ONE compiler: a derivation-mode run of the realized engine
      // plan (CMP-9) — the bespoke walk this session used to host is gone
      auto engine = load_engine(op.value("splice", nlohmann::json()));
      auto c = realized_compile(s, nullptr, engine, app);
      r = {{"execution", c.execution_cid},
           {"provenance", c.provenance_cid},
           {"map", c.execution["map"]},
           {"mappings", c.execution["mappings"]},
           {"passes_run", c.tick_order},
           {"memo", c.memo},
           {"structural_memo", c.structural_work == 0},
           {"engine_instances", executor::exec_plan::live_engine_plans()}};
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
      auto c = realized_compile(s, nullptr, load_engine(nlohmann::json()), app);
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
      auto c = realized_compile(s, nullptr, load_engine(nlohmann::json()), app);
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
    } else if (what == "app-cid") {
      r = {{"cid", s.put_node(app, false)}};
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
    } else {
      throw std::runtime_error("unknown compile op: " + what);
    }
    results.push_back(r);
  }
  std::cout << nlohmann::json{{"results", results}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
