#pragma once
#include <span>
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
// A binding is one blob: [ fn ][ slots ][ outmem ]. The slots array (first
// in_count are inputs, the rest outputs) lives INLINE right after fn, and the
// output slots point into outmem — the owned output cells, later in the same blob.
struct binding {
  word fn;
  void** slots() const { return (void**)(this + 1); }   // inline, immediately after fn
  void operator()() const { fn(slots()); }              // the type-ERASED twin of component::operator()
};

// The escapement: tick each binding in order. The movement is a span of pointers
// to self-owning bindings (each mallocs its own blob in make_binding), so we tick
// through the indirection.
void tick(std::span<binding* const> movement) {
  for (binding* b : movement) (*b)();
}

// A plan is a built movement ready to tick: the span of self-owning bindings.
// (The count that used to ride alongside is now the span's length.)
struct plan { std::span<binding*> movement; };

}
