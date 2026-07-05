#include "peer_session.hpp"

#include <fstream>
#include <iostream>
#include <map>

#include "exec_plan.hpp"
#include "parser/parser.hpp"
#include "phase.hpp"
#include "registry_face/registry_face.hpp"
#include "store.hpp"
#include "svalue_accessors.hpp"

namespace syg::harness {
namespace {

organs::graph_doc pristine_engine() {
  std::ifstream f("vocabulary/engine-v0.json");
  return organs::parse_graph(nlohmann::json::parse(f));
}

// descriptors commit as TYPE NODES through the canonical encoder
// (CMP-9.4): one committed declaration answers everyone
std::map<std::string, std::string> commit_types(store::peer_store& s) {
  std::map<std::string, std::string> cids;
  for (const auto* n : organs::registered_natives()) {
    nlohmann::ordered_json ports;
    for (const auto& p : n->in_ports)
      ports[p.name] = {{"dir", "in"}, {"kind", p.kind},
                       {"discipline", p.discipline}};
    for (const auto& p : n->out_ports)
      ports[p.name] = {{"dir", "out"}, {"kind", p.kind},
                       {"discipline", p.discipline}};
    cids[n->name] = s.put_node(
        {{"kind", "node-type"}, {"name", n->name}, {"ports", ports}}, false);
  }
  return cids;
}

}  // namespace

int peer_session(const nlohmann::json& in) {
  // ONE peer: its store, its (lazily instantiated) engine level, its
  // running app instances — every mutation an op, every read a resolution
  store::peer_store s("peer");
  auto type_cids = commit_types(s);
  std::unique_ptr<executor::exec_plan> engine;  // resident only when edited
  std::unique_ptr<executor::exec_plan> app_instance;
  nlohmann::json app;
  long engine_alive = 0;

  auto run_compile = [&](nlohmann::json& r) {
    auto engine_doc = engine ? engine->doc() : pristine_engine();
    auto app_cid = s.put_node(app, false);
    auto engine_cid = s.put_node(organs::serialize_graph(engine_doc), false);
    nlohmann::json recipe{{"op", "compile"},
                          {"inputs", {{"app", {{"/", app_cid}}},
                                      {"engine", {{"/", engine_cid}}}}},
                          {"determinism", "exact"}};
    if (auto hit = s.memo_lookup(recipe)) {
      r = {{"execution", hit->output}, {"memo", true}, {"passes_ticked", 0},
           {"engine_alive", engine_alive}};
      return;
    }
    // derivation-mode run of the ENGINE PLAN (EXE-7): transient unless the
    // editor holds the level open
    executor::exec_plan* plan = engine.get();
    std::unique_ptr<executor::exec_plan> transient;
    if (!plan) {
      transient = std::make_unique<executor::exec_plan>(engine_doc, 48000, 128);
      plan = transient.get();
      ++engine_alive;
    }
    // counters keyed by the PLAN's realized instances (post-expansion) —
    // a graph-authored pass is witnessed as aud0.t0, not its authored id
    auto instances = plan->instance_ids();
    std::map<std::string, long> before;
    for (const auto& id : instances)
      before[id] = std::max(plan->recomputes(id), 0L);
    abi::reset_compile_work();
    plan->submit({"set_text", "receive0/app", app.dump(), "peer"});
    std::vector<std::string> order;
    std::string result;
    for (int i = 0; i < 200; ++i) {
      plan->pump_block();
      if (plan->last_tick_order().size() > order.size())
        order = plan->last_tick_order();
      if (const auto* sv = plan->svalue_of("realize0/out"); sv && sv->value) {
        result = generated::as_text(*sv);
        if (!result.empty() && i > 20) break;
      }
    }
    if (result.empty()) throw std::runtime_error("the engine never realized");
    auto execution = nlohmann::json::parse(result);
    auto c = s.commit_derivation(recipe, execution);
    nlohmann::json ticks, ticks_total;
    long total = 0;
    for (const auto& id : instances) {
      auto abs_n = std::max(plan->recomputes(id), 0L);
      ticks[id] = abs_n - before[id];
      ticks_total[id] = abs_n;
      total += abs_n - before[id];
    }
    if (transient) {
      transient.reset();  // derivation mode: run to completion, release
      --engine_alive;
    }
    r = {{"execution", c.output},
         {"provenance", c.provenance},
         {"execution_body", execution},
         {"memo", false},
         {"pass_ticks", ticks},
         {"pass_ticks_total", ticks_total},
         {"passes_ticked", total},
         {"tick_order", order},
         {"outside_hook_work", abi::compile_work_outside_hooks()},
         {"engine_alive", engine_alive}};
  };

  auto results = nlohmann::json::array();
  for (const auto& op : in.value("ops", nlohmann::json::array())) {
    const std::string what = op.at("op");
    nlohmann::json r;
    if (what == "set-app") {
      app = op.at("graph");
      r = nullptr;
    } else if (what == "compile") {
      run_compile(r);
    } else if (what == "open-engine-editor") {
      if (!engine) {
        engine = std::make_unique<executor::exec_plan>(pristine_engine(),
                                                       48000, 128);
        ++engine_alive;
      }
      r = {{"engine_alive", engine_alive}};
    } else if (what == "close-engine-editor") {
      if (engine) {
        engine.reset();
        --engine_alive;
      }
      r = {{"engine_alive", engine_alive}};
    } else if (what == "engine-edit") {
      if (!engine) throw std::runtime_error("open the engine editor first");
      for (const auto& e : op.at("ops"))
        engine->submit({e.at("op"), e.value("a", ""), e.value("b", ""),
                        e.value("author", "peer")});
      engine->pump_block();  // the boundary applies the edits
      r = {{"engine_nodes", engine->doc().nodes.size()}};
    } else if (what == "engine-doc") {
      auto doc = engine ? engine->doc() : pristine_engine();
      r = {{"doc", organs::serialize_graph(doc)}};
    } else if (what == "render") {
      if (!app_instance)
        app_instance = std::make_unique<executor::exec_plan>(
            organs::parse_graph(app), 48000, 128);
      for (int i = 0; i < op.value("blocks", 10); ++i)
        app_instance->pump_block();
      r = {{"engine_alive", engine_alive}};
    } else if (what == "type-cid") {
      r = {{"cid", type_cids.at(op.at("type"))}};
    } else if (what == "commit-app") {
      // the lock is honest (CMP-9.4): real type cids, never placeholders
      auto doc = organs::parse_graph(op.at("graph"));
      nlohmann::ordered_json lock;
      for (const auto& [id, tname] : doc.nodes)
        lock[tname] = {{"/", type_cids.at(tname)}};
      doc.lock = lock;
      auto committed = organs::serialize_graph(doc);
      auto cid = s.put_node(committed, false);
      if (op.contains("ref")) s.bind_ref(op.at("ref"), cid);
      r = {{"cid", cid}, {"lock", lock}};
    } else if (what == "type-promises") {
      // resolver-side: walk the COMMITTED chain lock -> type -> ports
      const auto* gcid = s.ref(op.at("ref"));
      if (!gcid) throw std::runtime_error("no such ref");
      auto graph = s.get_node(*gcid);
      auto node_type = graph.at("topology").at("nodes")
                           .at(op.at("node").get<std::string>())
                           .at("type").get<std::string>();
      auto type_cid =
          graph.at("lock").at(node_type).at("/").get<std::string>();
      auto type_node = s.get_node(type_cid);
      r = {{"promises", type_node.at("ports").at(op.at("port").get<std::string>())},
           {"type_cid", type_cid},
           {"via", "committed declaration"}};
    } else if (what == "registry-promises") {
      // runtime-side: the SAME declaration, answered from the linked table
      for (const auto* n : organs::registered_natives())
        if (op.at("type") == n->name) {
          for (const auto& p : n->in_ports)
            if (op.at("port") == p.name)
              r = {{"promises", {{"dir", "in"}, {"kind", p.kind},
                                 {"discipline", p.discipline}}}};
          for (const auto& p : n->out_ports)
            if (op.at("port") == p.name)
              r = {{"promises", {{"dir", "out"}, {"kind", p.kind},
                                 {"discipline", p.discipline}}}};
        }
    } else {
      throw std::runtime_error("unknown peer op: " + what);
    }
    results.push_back(r);
  }
  std::cout << nlohmann::json{{"results", results}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
