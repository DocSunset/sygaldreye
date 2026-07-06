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
  json results = json::array();

  for (const auto& op : in.value("ops", json::array())) {
    const std::string what = op.at("op");
    json r;
    if (what == "open") {
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
