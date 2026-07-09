#pragma once
#include <cstddef>
#include "node.hpp"
#include "escapement.hpp"

namespace syg::esc {

std::size_t sum(std::size_t a, std::size_t b) { return a + b; }

// round x up to the next max_align_t boundary.
std::size_t align_up(std::size_t x) {
  std::size_t a = alignof(std::max_align_t);
  return (x + a - 1) / a * a;
}

// bump: carve one max-aligned allocation out of an arena; return its offset. The
// arena itself comes from the mem_malloc word — allocation stays vocabulary.
std::size_t bump(std::size_t* top, std::size_t bytes) {
  std::size_t at = align_up(*top);
  *top = at + bytes;
  return at;
}

// make_binding: build a self-owning binding in the arena. One region holds the
// binding, its input- and output-pointer arrays, and its output cells. Inputs
// start unwired (nullptr); each output points at an owned cell. The binding owns
// its output the way a component owns `T out` — just heap-sized, at runtime, from
// the descriptor, because the type was erased.
binding* make_binding(unsigned char* arena, std::size_t* top, const node* n) {
  std::size_t nslots = n->in_count + n->out_count;
  binding* b = (binding*)(arena + bump(top, sizeof(binding) + nslots * sizeof(void*)));  // [fn][slots]
  b->fn = n->fn;
  void** s = b->slots();
  for (std::size_t j = 0; j < n->in_count;  ++j) s[j] = nullptr;                  // inputs unwired
  for (std::size_t j = 0; j < n->out_count; ++j)
    s[n->in_count + j] = arena + bump(top, n->out_sizes[j]);                       // owned output cells
  return b;
}

// nop: a source that ticks to nothing — its owned output already holds its value.
void nop(void**) {}

// constant: a source node (0 inputs, 1 cell output). The crown seeds its output
// from the tape and nop leaves it be, so the owned cell IS the constant. Now that
// bindings own their outputs, a constant is just a node — no SET op needed.
node constant() {
  static constexpr std::size_t no_in[]    = { 0 };
  static constexpr std::size_t one_cell[] = { sizeof(cell), 0 };
  return { 0, no_in, 1, one_cell, nop };
}

}
