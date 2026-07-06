#include "edit_session.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>

#include "exec_plan.hpp"
#include "parser/parser.hpp"

namespace syg::harness {
namespace {

using nlohmann::json;

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
      // EDR-8.1: a probe mapping attached to an edge exposes its value stream
      // on the VALUES SURFACE (pull-observability) without altering region
      // inference. A probe is a subscription, not a splice — it reads the
      // edge's source port each block; the topology is untouched, so the
      // region partition is identical to the un-probed graph. (A naive probe
      // that spliced a node into the edge WOULD move a region — the whole
      // point of the criterion.)
      auto g = syg::organs::parse_graph(op.at("graph"));
      syg::executor::exec_plan plan(g, 48000, 128);
      const auto& reg = plan.regions();
      json streams = json::object();
      const int blocks = op.value("blocks", 8);
      json probes = op.at("edges");  // the edges to probe, by source port
      std::vector<std::string> srcs;
      for (const auto& e : probes) srcs.push_back(e.get<std::string>());
      for (const auto& s2 : srcs) streams[s2] = json::array();
      for (int i = 0; i < blocks; ++i) {
        plan.pump_block();
        for (const auto& s2 : srcs) {
          // the values surface carries value/event edges; an audio-stream
          // edge has no frame cell (it belongs to the spectral surface, the
          // other half of EDR-8) — expose it as null rather than crash.
          try {
            streams[s2].push_back(plan.value_of(s2));
          } catch (const std::exception&) {
            streams[s2].push_back(nullptr);
          }
        }
      }
      auto as_arr = [](const std::vector<std::string>& v) { return json(v); };
      r = {{"regions", {{"block", as_arr(reg.block)},
                        {"frame", as_arr(reg.frame)},
                        {"inert", as_arr(reg.inert)}}},
           {"streams", streams}};
    } else if (what == "regions") {
      // the un-probed baseline: region inference over the graph as authored.
      auto g = syg::organs::parse_graph(op.at("graph"));
      syg::executor::exec_plan plan(g, 48000, 128);
      const auto& reg = plan.regions();
      auto as_arr = [](const std::vector<std::string>& v) { return json(v); };
      r = {{"block", as_arr(reg.block)}, {"frame", as_arr(reg.frame)},
           {"inert", as_arr(reg.inert)}};
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
