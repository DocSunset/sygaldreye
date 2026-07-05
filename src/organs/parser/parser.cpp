// clause: machinery — the interchange decode organ
#include "parser/parser.hpp"

#include <stdexcept>

namespace syg::organs {

graph_doc parse_graph(const nlohmann::json& j) {
  graph_doc g;
  g.kind = j.at("kind");
  g.lock = j.value("lock", nlohmann::json::object());
  const auto& topo = j.at("topology");
  for (const auto& [id, rec] : topo.at("nodes").items())
    g.nodes.emplace_back(id, rec.at("type"));
  for (const auto& e : topo.value("edges", nlohmann::json::array()))
    g.edges.emplace_back(e.at("from"), e.at("to"));
  g.lift_key = topo.value("lift_key", "");
  g.defaults = j.value("defaults", nlohmann::json::object());
  for (const auto& [k, v] : j.items())
    if (k != "kind" && k != "lock" && k != "topology" && k != "defaults")
      throw std::runtime_error("not part of the persisted surface: " + k);
  return g;
}

nlohmann::ordered_json serialize_graph(const graph_doc& g) {
  nlohmann::ordered_json nodes, topo;
  for (const auto& [id, type] : g.nodes) nodes[id] = {{"type", type}};
  topo["nodes"] = nodes;
  nlohmann::ordered_json edges = nlohmann::json::array();
  for (const auto& [f, t] : g.edges) edges.push_back({{"from", f}, {"to", t}});
  topo["edges"] = edges;
  if (!g.lift_key.empty()) topo["lift_key"] = g.lift_key;
  return {{"kind", g.kind}, {"lock", g.lock},
          {"topology", topo}, {"defaults", g.defaults}};
}

}  // namespace syg::organs
