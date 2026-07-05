// clause: machinery — the interchange decode organ (ch. 16 stratum 3)
#pragma once
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace syg::organs {

// The composite graph's persisted surface (LNG-8): everything here
// round-trips; nothing derived belongs here.
struct graph_doc {
  std::string kind;
  nlohmann::ordered_json lock;      // name -> type cid (ADR-026)
  std::vector<std::pair<std::string, std::string>> nodes;  // id, type
  std::vector<std::pair<std::string, std::string>> edges;  // from, to
  std::string lift_key;             // empty = absent
  nlohmann::ordered_json defaults;  // route -> value
};

graph_doc parse_graph(const nlohmann::json& j);
nlohmann::ordered_json serialize_graph(const graph_doc& g);

}  // namespace syg::organs
