#pragma once
#include <cstddef>
#include <cstdlib>
#include "node.hpp"
#include "escapement.hpp"

namespace syg::esc {

std::size_t sum(std::size_t a, std::size_t b) { return a + b; }


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
