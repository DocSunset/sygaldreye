#include "edit_session.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>

#include <fstream>

#include "exec_plan.hpp"
#include "regions.hpp"
#include "parser/parser.hpp"
#include "realized_compile.hpp"
#include "store.hpp"

namespace syg::harness {
namespace {

using nlohmann::json;

organs::graph_doc load_engine() {
  std::ifstream f("vocabulary/engine-v0.json");
  return organs::parse_graph(json::parse(f));
}

// The realized view's mappings (EDR-4). `execution["mappings"]` IS the
// compiler-DERIVED boundary-adapter set (region_map.mappings — "derived;
// NEVER persisted"), computed by infer_regions, not authored: so being in
// this list is exactly what "compiler-inserted" means, and every such adapter
// is replaceable by an explicit mapping node (which is what EDR-4.1 does). An
// adapter that names an app-graph node the user placed is NOT compiler-
// inserted — it would be that node, absent from this derived list. The label
// is therefore derived, not stamped.
json labeled_mappings(store::peer_store& s, const json& app) {
  auto c = realized_compile(s, nullptr, load_engine(), app);
  json out = json::array();
  for (const auto& m : c.execution.value("mappings", json::array())) {
    json e = m;
    std::string kind = m.value("mapping", std::string("?"));
    // a derived adapter carries a mapping KIND and no authored node identity
    bool derived = !kind.empty() && kind != "?";
    e["compiler_inserted"] = derived;
    e["replaceable"] = derived;  // replace it with an explicit mapping node
    e["label"] = (derived ? "compiler-inserted " : "authored ") + kind;
    out.push_back(e);
  }
  return out;
}

// One gesture = a burst of attributed edit ops into the arbiter, applied at
// the tick boundary (exactly what a gesture NODE emits — EDR-7's whole point).
void apply_gesture(syg::executor::exec_plan& live, const json& ops) {
  for (const auto& e : ops)
    live.submit({e.at("op"), e.value("a", ""), e.value("b", ""),
                 e.value("author", "editor")});
  live.pump_block();  // the boundary applies them
}

}  // namespace

int edit_session(const nlohmann::json& in) {
  std::unique_ptr<syg::executor::exec_plan> live;
  std::unique_ptr<syg::executor::exec_plan> target;
  std::string arbiter_id;  // re-aimed after any rebuild (swap re-realizes)
  json results = json::array();

  for (const auto& op : in.value("ops", json::array())) {
    const std::string what = op.at("op");
    json r;
    if (what == "open-editor") {
      // EDR-1: the editor is nodes. A live editor plan whose palette (a
      // subgraph emitting op events) is aimed at a target's arbiter — the
      // graph-edits-graph shape (LNG-11.3). Swapping the palette subgraph
      // live changes what the editor spawns, no restart.
      live = std::make_unique<syg::executor::exec_plan>(
          syg::organs::parse_graph(op.at("editor")), 48000, 128);
      target = std::make_unique<syg::executor::exec_plan>(
          syg::organs::parse_graph(op.at("target")), 48000, 128);
      arbiter_id = op.at("arbiter").get<std::string>();
      live->point_arbiter(arbiter_id, *target);
      r = {{"editor_nodes", live->doc().nodes.size()},
           {"target_nodes", target->doc().nodes.size()}};
    } else if (what == "bang") {
      if (!live) throw std::runtime_error("open the editor first");
      live->post_event(op.at("route"), 0.0);
      for (int i = 0; i < op.value("settle", 20); ++i) {
        live->pump_block();
        if (target) target->pump_block();
      }
      r = {{"ok", true}};
    } else if (what == "swap") {
      // slot swap+migrate (EXE-5) of a node's TYPE, preserving its wiring:
      // remove the node, re-add it under the new type, restore the edges it
      // sat on. rebuild() migrates every surviving node's state by route.
      if (!live) throw std::runtime_error("open the editor first");
      const std::string id = op.at("id"), type = op.at("type");
      std::vector<std::pair<std::string, std::string>> touched;
      for (const auto& [f, t] : live->doc().edges)
        if (f.substr(0, f.find('/')) == id || t.substr(0, t.find('/')) == id)
          touched.push_back({f, t});
      live->submit({"remove_node", id, "", "editor"});
      live->submit({"add_node", id, type, "editor"});
      for (const auto& [f, t] : touched)
        live->submit({"add_edge", f, t, "editor"});
      live->pump_block();
      // the rebuild re-realized the plan — the arbiter inlet is a fresh
      // instance; re-aim it at the target (its pointer is not migrated state).
      if (target && !arbiter_id.empty()) live->point_arbiter(arbiter_id, *target);
      r = {{"editor_nodes", live->doc().nodes.size()}};
    } else if (what == "agent-drive") {
      // EDR-7: an agent drives every gesture through SOURCE NODES — each op
      // arrives as an op event from an op_button into the target's arbiter,
      // the same path a human's gesture node uses. No privileged agent API.
      // The editor here is {btn:button -> ob0:op_button -> arb0:arbiter_inlet}.
      live = std::make_unique<syg::executor::exec_plan>(
          syg::organs::parse_graph(op.at("editor")), 48000, 128);
      target = std::make_unique<syg::executor::exec_plan>(
          syg::organs::parse_graph(op.at("target")), 48000, 128);
      arbiter_id = op.at("arbiter").get<std::string>();
      live->point_arbiter(arbiter_id, *target);
      for (const auto& g : op.at("script")) {
        // load the source op_button with this gesture, then bang it
        live->submit({"set_text", op.at("op_route"), g.dump(), "agent"});
        live->pump_block();
        live->post_event(op.at("bang"), 0.0);
        for (int i = 0; i < 4; ++i) { live->pump_block(); target->pump_block(); }
      }
      r = {{"graph", syg::organs::serialize_graph(target->doc())}};
    } else if (what == "target-doc") {
      if (!target) throw std::runtime_error("no target");
      r = {{"graph", syg::organs::serialize_graph(target->doc())}};
    } else if (what == "open") {
      live = std::make_unique<syg::executor::exec_plan>(
          syg::organs::parse_graph(op.at("graph")), 48000, 128);
      for (int i = 0; i < op.value("warm", 8); ++i) live->pump_block();
      r = {{"nodes", live->doc().nodes.size()}};
    } else if (what == "gesture") {
      if (!live) throw std::runtime_error("open a graph first");
      apply_gesture(*live, op.at("ops"));
      // let modulation run a few blocks so a live cell would diverge from the
      // default if serialization ever leaked one (the EDR-2 trap)
      for (int i = 0; i < op.value("settle", 8); ++i) live->pump_block();
      r = {{"nodes", live->doc().nodes.size()}};
    } else if (what == "undo") {
      // one editor undo = one structural snapshot (EDR-3): param drift folds
      // into the surrounding structural step.
      if (!live) throw std::runtime_error("open a graph first");
      for (int i = 0; i < op.value("n", 1); ++i) live->undo_gesture();
      live->pump_block();
      r = {{"nodes", live->doc().nodes.size()}};
    } else if (what == "save" || what == "doc") {
      if (!live) throw std::runtime_error("open a graph first");
      // serialization captures the persisted surface — defaults, never the
      // live cell values (EXE-1.2 at the UX level).
      r = {{"graph", syg::organs::serialize_graph(live->doc())}};
    } else if (what == "probe") {
      // EDR-8.1: a probe attached to an edge exposes its value stream on the
      // VALUES SURFACE (pull-observability) and MUST NOT alter region
      // inference. The probe is a real, INERT observer node (a value tap with
      // no demanded output) attached to the edge's source. Region inference
      // (run WITH the probe present) places it inert and leaves the block and
      // frame partition of every original node untouched — a demanding probe
      // (a block sink) would instead pull its source into the block region,
      // which is exactly what this witness forbids.
      auto g = syg::organs::parse_graph(op.at("graph"));
      auto base = syg::executor::infer_regions(g);
      auto probed_doc = g;
      const int blocks = op.value("blocks", 8);
      std::vector<std::string> srcs, probe_ids;
      for (const auto& e : op.at("edges")) srcs.push_back(e.get<std::string>());
      int k = 0;
      for (const auto& s2 : srcs) {
        std::string id = "probe" + std::to_string(k++);
        probe_ids.push_back(id);
        probed_doc.nodes.push_back({id, "tmux"});      // an inert value tap
        probed_doc.edges.push_back({s2, id + "/in"});  // attached to the edge
      }
      auto probed = syg::executor::infer_regions(probed_doc);
      // the value stream comes from a plain render of the UNPROBED graph — the
      // probe is a read, so no probe node is ever built into a live plan
      syg::executor::exec_plan plan(g, 48000, 128);
      json streams = json::object();
      for (const auto& s2 : srcs) streams[s2] = json::array();
      for (int i = 0; i < blocks; ++i) {
        plan.pump_block();
        for (const auto& s2 : srcs) {
          try {  // audio edges have no frame cell — the spectral surface
            streams[s2].push_back(plan.value_of(s2));
          } catch (const std::exception&) {
            streams[s2].push_back(nullptr);
          }
        }
      }
      auto as_arr = [](const std::vector<std::string>& v) { return json(v); };
      r = {{"base", {{"block", as_arr(base.block)}, {"frame", as_arr(base.frame)},
                     {"inert", as_arr(base.inert)}}},
           {"probed", {{"block", as_arr(probed.block)},
                       {"frame", as_arr(probed.frame)},
                       {"inert", as_arr(probed.inert)}}},
           {"probe_ids", as_arr(probe_ids)},
           {"streams", streams}};
    } else if (what == "regions") {
      // region inference over the graph as authored (the un-probed baseline).
      auto g = syg::organs::parse_graph(op.at("graph"));
      auto reg = syg::executor::infer_regions(g);
      auto as_arr = [](const std::vector<std::string>& v) { return json(v); };
      r = {{"block", as_arr(reg.block)}, {"frame", as_arr(reg.frame)},
           {"inert", as_arr(reg.inert)}};
    } else if (what == "realized") {
      // EDR-4: open the realized (execution) view; the compiler-inserted
      // mappings (the latch) are visible, labeled, and replaceable.
      store::peer_store s("editor");
      r = {{"mappings", labeled_mappings(s, op.at("graph"))}};
    } else if (what == "value") {
      if (!live) throw std::runtime_error("open a graph first");
      r = {{"value", live->value_of(op.at("route"))}};
    } else {
      r = {{"error", "unknown op"}, {"op", what}};
    }
    results.push_back(r);
  }
  std::cout << json{{"results", results}}.dump() << "\n";
  return 0;
}

}  // namespace syg::harness
