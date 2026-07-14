// syg::graph — a runtime-assembled graph as a syg_type. Hand-assembles a tiny two-node
// arena (no edges yet — edges need components with const T* inputs) and runs the whole
// lifecycle over it: place, tick, move, erase. assert-based; exit 0 passes.
#include "graph.hpp"
#include "stage0.hpp"
#include <cassert>
#include <cstdint>
using namespace syg;

int main() {
  // two pure-data value nodes: an int32 at offset 0, a float at offset 4. no edges.
  const syg_type_t* ti = stage0::generate_value<std::int32_t>();
  const syg_type_t* tf = stage0::generate_value<float>();
  graph::node ns[] = { { ti, 0 }, { tf, 4 } };
  graph::topology tp{ 2, ns, 0, nullptr, 8 };

  // a graph syg_type: size = arena, methods = the graph lifecycle, impl.data = the topology.
  syg_type_t gt{};
  gt.size = tp.arena_size; gt.align = 4;
  gt.place = graph::place; gt.tick = graph::tick; gt.move = graph::move; gt.erase = graph::erase;
  gt.impl = { nullptr, &tp };            // non-null impl.data ⇒ a graph

  alignas(4) unsigned char buf[8];
  syg_value_t g{ &gt, buf };
  g.type->place(g, nullptr);             // places each node into its arena slot
  *reinterpret_cast<std::int32_t*>(buf) = 7;
  *reinterpret_cast<float*>(buf + 4) = 2.5f;
  g.type->tick(g);                       // ticks each node (no-ops here) — exercises iteration
  assert(*reinterpret_cast<std::int32_t*>(buf) == 7 && *reinterpret_cast<float*>(buf + 4) == 2.5f);

  // move: migrate the arena node-by-node into a fresh instance, then re-bind (no edges).
  alignas(4) unsigned char buf2[8];
  syg_value_t g2{ &gt, buf2 };
  g.type->move(g2, buf);
  assert(*reinterpret_cast<std::int32_t*>(buf2) == 7 && *reinterpret_cast<float*>(buf2 + 4) == 2.5f);

  g.type->erase(g2);
  g.type->erase(g);
  return 0;
}
