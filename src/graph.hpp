#pragma once
// graph.hpp — a runtime-assembled graph as a syg_type (stage 0).
//
// An instance is one arena (grph.data); each node's state sits at a fixed offset. The
// topology (shared, behind the graph-type's impl.data) describes wiring in indices, so it
// is base-independent and drives every instance. The type's members point at the dangling
// endpoints (unwired inputs = inlets, unconsumed outputs = outlets). Reasoning: ../stage0.md.
#include "syg.hpp"
#include <cstdint>
#include <span>

namespace syg::graph {

struct node { const syg_type_t* type; std::uint32_t offset; };     // a node placed at arena + offset
struct edge { std::uint32_t src_node, src_ep, dst_node, dst_ep; }; // endpoints: indices into each node's members
struct topology {                                                  // lives behind the graph-type's impl.data
  std::uint32_t node_count; const node* nodes;                     // stored in tick (topo) order
  std::uint32_t edge_count; const edge* edges;
  std::uint32_t arena_size;                                        // = the instance's size
};

// syg_values are type-erased, so a prefix tracks what one holds: grph (a graph instance),
// tplg (a topology), n (a node's value), ep (an endpoint). Concrete structs keep plain names.
inline const topology* topo(syg_value_t grph) { return static_cast<const topology*>(grph.type->impl.data); }

inline std::span<const node> nodes(syg_value_t grph) { auto* tplg = topo(grph); return { tplg->nodes, tplg->node_count }; }
inline std::span<const edge> edges(syg_value_t grph) { auto* tplg = topo(grph); return { tplg->edges, tplg->edge_count }; }

template <class T, class F> void each(std::span<const T> s, F fn) { for (auto& x : s) fn(x); }   // the only loop

inline syg_value_t val(syg_value_t grph, const node& n) { return { n.type, static_cast<char*>(grph.data) + n.offset }; }

inline void wire(syg_value_t grph, const edge& e) {                // bind one edge — typed store lives in the node
  auto* tplg = topo(grph); char* a = static_cast<char*>(grph.data);
  auto& s = tplg->nodes[e.src_node]; auto& d = tplg->nodes[e.dst_node];
  syg_value_t dv = val(grph, d);                                   // the dst node's value
  void* src = a + s.offset + s.type->members[e.src_ep].offset;     // &(the src output cell)
  dv.type->wire(dv, e.dst_ep, src);                                // dv's input ep <- src, legally
}
inline void bind(syg_value_t grph) { each(edges(grph), [&](const edge& e){ wire(grph, e); }); }

inline void place(syg_value_t grph, void**) {
  each(nodes(grph), [&](const node& n){ syg_value_t nv = val(grph, n); nv.type->place(nv, nullptr); });
  bind(grph);
}
inline void tick(syg_value_t grph)  { each(nodes(grph), [&](const node& n){ syg_value_t nv = val(grph, n); nv.type->tick(nv); }); }
inline void erase(syg_value_t grph) { each(nodes(grph), [&](const node& n){ syg_value_t nv = val(grph, n); nv.type->erase(nv); }); }
inline void move(syg_value_t grph, void* src) {
  each(nodes(grph), [&](const node& n){ syg_value_t nv = val(grph, n); nv.type->move(nv, static_cast<char*>(src) + n.offset); });
  bind(grph);
}

}  // namespace syg::graph
