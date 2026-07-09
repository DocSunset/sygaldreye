#pragma once
#include "cell.hpp"
#include "node.hpp"

namespace syg::esc {

// A binding is the binding role reified: it binds behavior to data — a word
// (resolved to its value at plan time; re-resolved when the plan is rebuilt on a
// swap, which is how the book gets liveness) closed over pointers to its input
// cells in state and the output cell it writes. Literally a closure — captured by
// reference into shared state, which is why inputs must stay const and why fan-out
// works. The instance is NOT here: it emerges from state × movement, existence by
// reference (L10).
struct binding {
  void** slots;                                       // first in_count are inputs, the rest outputs
  word   fn;
  void operator()() const { fn(slots); }              // the type-ERASED twin of component::operator()
};

// The escapement: tick each binding in order. The movement is an array of
// pointers to self-owning bindings (each carved from the arena by make_binding),
// so we tick through the indirection.
void tick(std::size_t steps, binding* const* movement) {
  for (std::size_t i = 0; i < steps; ++i)
    (*movement[i])();
}

// A plan is a built movement ready to tick: the array of self-owning bindings and
// how many. (tick's two arguments, bundled.)
struct plan { binding** movement; std::size_t steps; };

}
