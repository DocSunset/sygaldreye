#include "regions.hpp"

#include <map>
#include <set>

#include "oracle/oracle.hpp"
#include "registry_face/registry_face.hpp"

namespace syg::executor {
namespace {

const crown::port_decl* find_port(const std::vector<crown::port_decl>& ports,
                                  const std::string& name) {
  for (const auto& p : ports)
    if (name == p.name) return &p;
  return nullptr;
}

}  // namespace

region_map infer_regions(const organs::graph_doc& g) {
  region_map out;
  std::map<std::string, const crown::native_type*> types;
  for (const auto& [id, tname] : g.nodes) {
    for (const auto* n : organs::registered_natives())
      if (tname == n->name) types[id] = n;
    if (!types.count(id))
      out.errors.push_back("unknown node type: " + tname + " (" + id + ")");
  }
  // sinks: block-discipline inputs and no outputs (the dac shape)
  std::set<std::string> block;
  for (const auto& [id, t] : types) {
    bool block_in = false;
    for (const auto& p : t->in_ports)
      if (std::string(p.discipline) == "block") block_in = true;
    if (block_in && t->out_ports.empty()) block.insert(id);
  }
  // upstream closure through AUDIO edges (kind == audio on the source port)
  bool grew = true;
  while (grew) {
    grew = false;
    for (const auto& [from, to] : g.edges) {
      auto src_id = from.substr(0, from.find('/'));
      auto dst_id = to.substr(0, to.find('/'));
      if (!block.count(dst_id) || block.count(src_id)) continue;
      if (!types.count(src_id) || !types.count(dst_id)) continue;
      const auto* sp = find_port(types[src_id]->out_ports,
                                 from.substr(from.find('/') + 1));
      // stream edges at the block clock carry membership upstream —
      // "audio edges" generalized to the discipline (a scalar stream at
      // block cadence is block-region work all the same)
      if (sp && std::string(sp->discipline) == "block")
        block.insert(src_id), grew = true;
    }
  }
  for (const auto& [id, tname] : g.nodes)
    (block.count(id) ? out.block : out.frame).push_back(id);
  // the inert lint (EXE-11.3): demand flows backward from the block region
  std::set<std::string> demanded(block.begin(), block.end());
  bool more = true;
  while (more) {
    more = false;
    for (const auto& [from, to] : g.edges) {
      auto src = from.substr(0, from.find('/'));
      auto dst = to.substr(0, to.find('/'));
      if (demanded.count(dst) && !demanded.count(src))
        demanded.insert(src), more = true;
    }
  }
  for (const auto& id : out.frame)
    if (!demanded.count(id) && types.count(id) && !types[id]->clocked)
      out.inert.push_back(id);
  // boundary mappings: the one promise oracle, per edge
  for (std::size_t i = 0; i < g.edges.size(); ++i) {
    const auto& [from, to] = g.edges[i];
    auto src_id = from.substr(0, from.find('/'));
    auto dst_id = to.substr(0, to.find('/'));
    if (!types.count(src_id) || !types.count(dst_id)) continue;
    const auto* sp = find_port(types[src_id]->out_ports,
                               from.substr(from.find('/') + 1));
    const auto* dp = find_port(types[dst_id]->in_ports,
                               to.substr(to.find('/') + 1));
    if (!sp || !dp) {
      out.errors.push_back("no such port on edge " + std::to_string(i));
      continue;
    }
    auto v = naming::connection_legal({sp->kind, sp->discipline},
                                      {dp->kind, dp->discipline});
    if (!v.legal) {
      out.errors.push_back("illegal edge " + std::to_string(i) + ": " + from +
                           " -> " + to);
    } else if (types.count(dst_id) && types[dst_id]->is_mapping) {
      // a mapping already sits at this boundary: insert nothing (L13)
    } else if (!v.mapping.empty()) {
      auto mapping = v.mapping;
      if (mapping == "snapshot" && block.count(dst_id))
        mapping = "latch";  // the consumer is clocked: deliver at its boundary
      out.mappings.push_back({i, mapping});
    } else if (block.count(src_id) && !block.count(dst_id)) {
      // a region crossing with matching promises still needs its mapping:
      // the frame side sees the last COMPLETED block (ch. 4, snapshot)
      out.mappings.push_back({i, "snapshot"});
    }
  }
  return out;
}

}  // namespace syg::executor
