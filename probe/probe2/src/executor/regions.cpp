// clause: machinery — executor package (plans, pacing, mappings)
#include "regions.hpp"

#include "phase.hpp"

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
  abi::note_compile_work();
  region_map out;
  std::map<std::string, const crown::native_type*> types;
  for (const auto& [id, tname] : g.nodes) {
    for (const auto* n : organs::all_natives())
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


// The schedule (2026-07-05 audit fix): Tarjan runs on the CUT edge set
// (cycle breaking); the condensation orders by ALL edges as hard
// constraints where acyclic and SOFT where a cut edge closes a loop —
// Kahn drains hard-ready components in index order, so the result never
// depends on node names.
scc_schedule scc_order(
    std::size_t n,
    const std::vector<std::pair<std::size_t, std::size_t>>& cycle_edges,
    const std::vector<std::pair<std::size_t, std::size_t>>& order_edges) {
  std::vector<std::vector<std::size_t>> adj(n);
  for (const auto& [s2, d] : cycle_edges) adj[s2].push_back(d);
  std::vector<int> comp(n, -1), low(n), num(n, -1);
  std::vector<std::size_t> stk;
  std::vector<bool> on(n, false);
  int counter = 0, ncomp = 0;
  auto dfs = [&](auto&& self, std::size_t v) -> void {
    num[v] = low[v] = counter++;
    stk.push_back(v);
    on[v] = true;
    for (auto w : adj[v]) {
      if (num[w] < 0) {
        self(self, w);
        low[v] = std::min(low[v], low[w]);
      } else if (on[w]) {
        low[v] = std::min(low[v], num[w]);
      }
    }
    if (low[v] == num[v]) {
      while (true) {
        auto w = stk.back();
        stk.pop_back();
        on[w] = false;
        comp[w] = ncomp;
        if (w == v) break;
      }
      ++ncomp;
    }
  };
  for (std::size_t v = 0; v < n; ++v)
    if (num[v] < 0) dfs(dfs, v);
  auto nc = static_cast<std::size_t>(ncomp);
  // hard constraints: the acyclic (cut) set — Kahn can always drain them.
  // soft constraints: every remaining edge — honored unless a cycle of
  // components (via cut edges) forces a deterministic index-order break.
  std::vector<std::set<int>> hard(nc), soft(nc);
  std::vector<int> hard_in(nc, 0), soft_in(nc, 0);
  for (const auto& [s2, d] : cycle_edges)
    if (comp[s2] != comp[d] &&
        hard[static_cast<std::size_t>(comp[s2])].insert(comp[d]).second)
      ++hard_in[static_cast<std::size_t>(comp[d])];
  for (const auto& [s2, d] : order_edges) {
    auto cs = comp[s2], cd = comp[d];
    if (cs == cd) continue;
    if (hard[static_cast<std::size_t>(cs)].count(cd)) continue;
    if (soft[static_cast<std::size_t>(cs)].insert(cd).second)
      ++soft_in[static_cast<std::size_t>(cd)];
  }
  std::vector<std::vector<std::size_t>> members(nc);
  for (std::size_t v = 0; v < n; ++v)
    members[static_cast<std::size_t>(comp[v])].push_back(v);
  std::vector<bool> self_loop_by_comp(nc, false);
  for (const auto& [s2, d] : cycle_edges)
    if (s2 == d) self_loop_by_comp[static_cast<std::size_t>(comp[s2])] = true;
  scc_schedule out;
  std::vector<bool> done(nc, false);
  for (std::size_t emitted = 0; emitted < nc; ++emitted) {
    int pick = -1;
    for (std::size_t c = 0; c < nc; ++c)  // fully-ready first, index order
      if (!done[c] && hard_in[c] == 0 && soft_in[c] == 0) {
        pick = static_cast<int>(c);
        break;
      }
    if (pick < 0)  // a cut-edge cycle: break it deterministically
      for (std::size_t c = 0; c < nc; ++c)
        if (!done[c] && hard_in[c] == 0) {
          pick = static_cast<int>(c);
          break;
        }
    if (pick < 0) throw std::runtime_error("the block schedule is stuck");
    auto pc = static_cast<std::size_t>(pick);
    done[pc] = true;
    out.components.push_back(members[pc]);
    out.self_loop.push_back(self_loop_by_comp[pc]);
    for (auto d2 : hard[pc]) --hard_in[static_cast<std::size_t>(d2)];
    for (auto d2 : soft[pc]) --soft_in[static_cast<std::size_t>(d2)];
  }
  return out;

}

}  // namespace syg::executor
