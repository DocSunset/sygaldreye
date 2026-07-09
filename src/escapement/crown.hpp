#pragma once
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <span>
#include <vector>
#include "plan.hpp"

namespace syg::esc {

// A tape op — one edit that builds the graph. Instance ids are NODE order.
//   'N': a = type (dict index), b = seed value for output[0] if the type is a source (0 inputs)
//   'L': a = producer, b = producer output index, c = consumer, d = consumer input slot
struct op { char kind; std::size_t a, b, c, d; };

// crown: replay the tape into a movement of self-owning bindings. NODE builds one
// (make_binding), seeding a source node's output from the tape; LINK points a
// consumer input at a producer output. No arena, no offset math, no SET — a
// constant is just a source node that owns its value (probe1, erased).
plan crown(const node* dict, std::span<const op> tape) {
  std::size_t steps = 0;
  for (const op& o : tape) if (o.kind == 'N') steps = sum(steps, 1);

  binding** mv = (binding**)std::malloc(steps * sizeof(binding*));  // the movement
  std::vector<std::size_t> nin(steps);                   // TRANSIENT scratch: producer in_counts

  std::size_t ni = 0;
  for (const op& o : tape) {
    if (o.kind == 'N') {
      mv[ni] = make_binding(&dict[o.a]);
      node d = dict[o.a];
      nin[ni] = d.in_sizes.size();                       // a producer's outputs start at slots[in_count]
      if (d.in_sizes.empty() && !d.out_sizes.empty()) {  // a source: seed output[0]
        cell v = (cell)o.b;
        std::size_t w = sizeof(cell) < d.out_sizes[0] ? sizeof(cell) : d.out_sizes[0];
        std::memcpy(mv[ni]->slots()[0], &v, w);
      }
      ni = sum(ni, 1);
    }
    if (o.kind == 'L')                                   // producer output idx = slots[its in_count + idx]
      mv[o.c]->slots()[o.d] = mv[o.a]->slots()[nin[o.a] + o.b];
  }
  return plan{ {mv, steps} };
}

// drop: the crown's inverse — free each self-owning binding, then the movement.
// (Whole-plan teardown; the applier will do this per-op on unlink.)
void drop(plan p) {
  for (binding* b : p.movement) std::free(b);
  std::free(p.movement.data());
}

}
