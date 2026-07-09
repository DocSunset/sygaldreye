#pragma once
#include <cstddef>
#include <cstdlib>
#include "node.hpp"
#include "escapement.hpp"

namespace syg::esc {

std::size_t sum(std::size_t a, std::size_t b) { return a + b; }

// make_binding: a self-owning binding — one heap blob, [fn][slots][outmem]. Inputs
// start unwired (nullptr); each output points at an owned cell just past the slot
// array. The binding owns its output the way a component owns `T out`; the only
// difference is placement, forced by erasure — the size isn't known until runtime.
// (Outputs pack flat after the 8-aligned slot array: fine for cell-shaped cells.)
binding* make_binding(const node* n) {
  std::size_t nin = n->in_sizes.size(), nout = n->out_sizes.size(), outbytes = 0;
  for (std::size_t sz : n->out_sizes) outbytes += sz;
  binding* b = (binding*)std::malloc(sizeof(binding) + (nin + nout) * sizeof(void*) + outbytes);
  b->fn = n->fn;
  void** s = b->slots();
  unsigned char* out = (unsigned char*)(s + nin + nout);
  for (std::size_t j = 0; j < nin;  ++j) s[j] = nullptr;                          // inputs unwired
  for (std::size_t j = 0; j < nout; ++j) { s[nin + j] = out; out += n->out_sizes[j]; }
  return b;
}

// nop: a source that ticks to nothing — its owned output already holds its value.
void nop(void**) {}

// constant: a source node (0 inputs, 1 cell output). The crown seeds its output
// from the tape and nop leaves it be, so the owned cell IS the constant. Now that
// bindings own their outputs, a constant is just a node — no SET op needed.
node constant() {
  static constexpr std::size_t one_cell[] = { sizeof(cell) };
  return { {}, one_cell, nop };   // no inputs (empty span), one owned cell output
}

}
