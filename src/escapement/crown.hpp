#pragma once
#include <cstddef>
#include <cstring>
#include "plan.hpp"

namespace syg::esc {

// A tape op — one edit that builds the graph. Instance ids are NODE order.
//   'N': a = type (dict index), b = seed value for output[0] if the type is a source (0 inputs)
//   'L': a = producer, b = producer output index, c = consumer, d = consumer input slot
struct op { char kind; std::size_t a, b, c, d; };

// crown: replay the tape into an arena of self-owning bindings. NODE builds one
// (make_binding), seeding a source node's output from the tape; LINK points a
// consumer input at a producer output. No flat state buffer, no offset math, no
// SET — a constant is just a source node that owns its value (probe1, erased).
plan crown(unsigned char* arena, [[maybe_unused]] std::size_t arena_size,
           const node* dict, [[maybe_unused]] std::size_t dict_count,
           const op* tape, std::size_t tape_len) {
  std::size_t steps = 0;
  std::size_t k = 0;
  while (k < tape_len) { if (tape[k].kind == 'N') steps = sum(steps, 1); k = sum(k, 1); }

  std::size_t top = 0;
  binding**    mv  = (binding**)(arena + bump(&top, steps * sizeof(binding*)));
  std::size_t* nin = (std::size_t*)(arena + bump(&top, steps * sizeof(std::size_t)));  // per-instance in_count

  std::size_t ni = 0;
  k = 0;
  while (k < tape_len) {
    op o = tape[k];
    if (o.kind == 'N') {
      mv[ni] = make_binding(arena, &top, &dict[o.a]);
      node d = dict[o.a];
      nin[ni] = d.in_count;                              // remember it: a producer's outputs start here
      if (d.in_count == 0 && d.out_count > 0) {          // a source: seed output[0] (= slots[in_count])
        cell v = (cell)o.b;
        std::size_t w = sizeof(cell) < d.out_sizes[0] ? sizeof(cell) : d.out_sizes[0];
        std::memcpy(mv[ni]->slots[d.in_count], &v, w);
      }
      ni = sum(ni, 1);
    }
    if (o.kind == 'L')                                   // producer output idx = slots[its in_count + idx]
      mv[o.c]->slots[o.d] = mv[o.a]->slots[nin[o.a] + o.b];
    k = sum(k, 1);
  }
  plan result;
  result.movement = mv;
  result.steps = steps;
  return result;
}

}
