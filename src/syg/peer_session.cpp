#include <dlfcn.h>

#include "peer_session.hpp"

#include <fstream>
#include <iostream>
#include <map>

#include "exec_plan.hpp"
#include "parser/parser.hpp"
#include "phase.hpp"
#include "realized_compile.hpp"
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
  // the census is the EXECUTOR's (CMP-6.1's witness) — no hand tally here
  auto engine_alive = [] { return executor::exec_plan::live_engine_plans(); };

  auto run_compile = [&](nlohmann::json& r) {
    auto engine_doc = engine ? engine->doc() : pristine_engine();
    auto c = realized_compile(s, engine.get(), engine_doc, app);
    r = {{"execution", c.execution_cid},
         {"memo", c.memo},
         {"engine_alive", engine_alive()}};
    if (c.memo) {
      r["passes_ticked"] = 0;
      return;
    }
    r["provenance"] = c.provenance_cid;
    r["execution_body"] = c.execution;
    r["pass_ticks"] = c.pass_ticks;
    r["pass_ticks_total"] = c.pass_ticks_total;
    r["passes_ticked"] = c.passes_ticked;
    r["tick_order"] = c.tick_order;
    r["outside_hook_work"] = c.outside_hook_work;
    r["structural_work"] = c.structural_work;
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
        engine->set_store(&s);
      }
      r = {{"engine_alive", engine_alive()}};
    } else if (what == "close-engine-editor") {
      engine.reset();
      r = {{"engine_alive", engine_alive()}};
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
      r = {{"engine_alive", engine_alive()}};
    } else if (what == "load-plugin") {
      // the plugin gate's mechanical half (AUT-5): a .so's node type joins
      // the registry; MSH-5 adds the trust dial later
      void* lib = dlopen(op.at("so").get<std::string>().c_str(),
                         RTLD_NOW | RTLD_LOCAL);
      if (!lib) throw std::runtime_error(std::string("dlopen: ") + dlerror());
      auto entry = reinterpret_cast<const crown::native_type* (*)()>(
          dlsym(lib, "syg_plugin_native"));
      if (!entry)
        throw std::runtime_error("the plugin lacks syg_plugin_native");
      auto* t = new crown::native_type(*entry());  // session-lifetime
      if (op.contains("as"))
        t->name = (new std::string(op.at("as").get<std::string>()))->c_str();
      organs::register_plugin_native(t);
      r = {{"registered", t->name}};
    } else if (what == "render-take") {
      // capture the sink for golden comparison (AUT-5.1) — hex f32le
      executor::exec_plan take(organs::parse_graph(app), 48000, 128);
      int blocks = op.value("blocks", 375);
      static const char* hx = "0123456789abcdef";
      std::string hex;
      hex.reserve(static_cast<std::size_t>(blocks) * 128 * 8);
      for (int i = 0; i < blocks; ++i) {
        const float* b = take.pump_block();
        const auto* c = reinterpret_cast<const unsigned char*>(b);
        for (std::size_t j = 0; j < 128 * sizeof(float); ++j) {
          hex += hx[c[j] >> 4];
          hex += hx[c[j] & 15];
        }
      }
      r = {{"hex", hex}};
    } else if (what == "app-cid") {
      r = {{"cid", s.put_node(app, false)}};
    } else if (what == "cat") {
      auto obj = s.get(op.at("cid"));
      if (!obj) throw std::runtime_error("no such object");
      r = {{"bytes", std::string(obj->begin(), obj->end())}};
    } else if (what == "unfreeze") {
      // ADR-014: unfreezing IS reading provenance — artifact -> execution
      // -> recipe -> the source graph's hash; the artifact carries nothing
      std::string app_cid;
      for (const auto& holder : s.backlinks(op.at("artifact")))
        for (const auto& prov : s.backlinks(holder)) {
          auto rec = s.get_node(prov);
          if (rec.contains("inputs") && rec["inputs"].contains("app"))
            app_cid = rec["inputs"]["app"]["/"];
        }
      if (app_cid.empty())
        throw std::runtime_error("no provenance path from the artifact");
      r = {{"app", app_cid}};
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
      for (const auto* n : organs::all_natives())
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
