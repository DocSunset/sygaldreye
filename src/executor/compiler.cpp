// clause: scaffolding (dissolves: CMP-9.2) — a bespoke walk over the
// engine doc; ADR-034 retro-mark: compilation becomes a derivation-mode
// run of the REALIZED engine plan, and this walk retires
#include "compiler.hpp"

#include <fstream>
#include <set>
#include <stdexcept>

#include "lift.hpp"
#include "regions.hpp"
#include "subgraph/subgraph.hpp"

namespace syg::executor {
namespace {

std::optional<nlohmann::json> graphs_dir(const std::string& type) {
  std::ifstream f("graphs/" + type + ".json");
  if (!f) return std::nullopt;
  return nlohmann::json::parse(f);
}

// engine topo order — pass execution order IS the wiring (CMP-3.2, L14)
std::vector<std::pair<std::string, std::string>> engine_order(
    const organs::graph_doc& e) {
  std::vector<std::pair<std::string, std::string>> order;
  std::set<std::string> done;
  while (order.size() < e.nodes.size()) {
    bool grew = false;
    for (const auto& [id, type] : e.nodes) {
      if (done.count(id)) continue;
      bool ready = true;
      for (const auto& [f, t] : e.edges)
        if (t.substr(0, t.find('/')) == id &&
            !done.count(f.substr(0, f.find('/'))))
          ready = false;
      if (ready) order.emplace_back(id, type), done.insert(id), grew = true;
    }
    if (!grew) throw std::runtime_error("engine graph has a cycle");
  }
  return order;
}

}  // namespace

compilation compile(const nlohmann::json& app_interchange,
                    const organs::graph_doc& engine, store::peer_store& s) {
  compilation out;
  auto app_cid = s.put_node(app_interchange, false);
  // re-serialize the engine so its identity joins the recipe
  nlohmann::ordered_json engine_ser = organs::serialize_graph(engine);
  auto engine_cid = s.put_node(engine_ser, false);
  nlohmann::json recipe{{"op", "compile"},
                        {"inputs", {{"app", {{"/", app_cid}}},
                                    {"engine", {{"/", engine_cid}}}}},
                        {"determinism", "exact"}};
  if (auto hit = s.memo_lookup(recipe)) {
    out.execution_cid = hit->output;
    out.provenance_cid = hit->provenance;
    out.execution = s.get_node(hit->output);
    out.map = out.execution.at("map");
    out.memo = true;
    out.structural_memo = true;
    return out;
  }
  // the STRUCTURAL stage keys on topology + lock alone (CMP-1.2)
  auto app_doc = organs::parse_graph(app_interchange);
  nlohmann::ordered_json topo_only, nodes;
  for (const auto& [id, t] : app_doc.nodes) nodes[id] = {{"type", t}};
  topo_only["nodes"] = nodes;
  auto edges = nlohmann::json::array();
  for (const auto& [f, t] : app_doc.edges)
    edges.push_back({{"from", f}, {"to", t}});
  topo_only["edges"] = edges;
  nlohmann::json structural_recipe{
      {"op", "compile-structure"},
      {"inputs", {{"topology", {{"/", s.put_node(topo_only, false)}}},
                  {"engine", {{"/", engine_cid}}}}},
      {"determinism", "exact"}};
  nlohmann::json structure;
  if (auto hit = s.memo_lookup(structural_recipe)) {
    structure = s.get_node(hit->output);
    out.structural_memo = true;
  }
  // run the passes in the engine's wiring order (the probe records them)
  region_map regions;
  organs::graph_doc expanded;
  for (const auto& [id, type] : engine_order(engine)) {
    out.passes_run.push_back(id);
    if (out.structural_memo &&
        (type == "recognize-region" || type == "fanin" ||
         type == "audio-region-rules"))
      continue;  // the structural stage came from the memo
    if (type == "recognize-region") {
      expanded = organs::expand_subgraphs(lift_expand(app_doc, graphs_dir),
                                          graphs_dir);
      regions = infer_regions(expanded);
      if (!regions.errors.empty())
        throw std::runtime_error("compile: " + regions.errors.front());
    }
    // receive/fanin/context/choose/realize: annotations at this rung; the
    // structure they publish is carried below
  }
  if (!out.structural_memo) {
    nlohmann::ordered_json maps = nlohmann::json::array();
    for (const auto& m : regions.mappings)
      maps.push_back({{"edge", m.edge}, {"mapping", m.mapping}});
    nlohmann::ordered_json xnodes;
    std::set<std::string> in_block(regions.block.begin(), regions.block.end());
    nlohmann::ordered_json map;
    for (const auto& [id, t] : expanded.nodes) {
      std::string region = in_block.count(id) ? "block" : "frame";
      xnodes[region + "/" + id] = {{"type", t}};
      // every app instance appears exactly once (CMP-2.2); lifted clones
      // fold back to their app node through the '#' key
      auto app_route = "nodes/" + id.substr(0, id.find('#'));
      if (!map.contains(app_route)) map[app_route] = region + "/" + id;
    }
    structure = {{"nodes", xnodes}, {"mappings", maps}, {"map", map}};
    s.commit_derivation(structural_recipe, structure);
  }
  nlohmann::json execution{{"kind", "execution-graph"},
                           {"nodes", structure["nodes"]},
                           {"mappings", structure["mappings"]},
                           {"map", structure["map"]},
                           {"defaults", app_doc.defaults},
                           {"backend", "interpret"}};
  auto c = s.commit_derivation(recipe, execution);
  out.execution_cid = c.output;
  out.provenance_cid = c.provenance;
  out.execution = execution;
  out.map = structure["map"];
  out.map_cid = s.put_node(structure["map"], false);
  return out;
}

}  // namespace syg::executor
