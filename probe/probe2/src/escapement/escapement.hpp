#pragma once
// The escapement (ADR-028, COR-1): the node contract and tick-in-order.
// A calling convention and a for-loop — freestanding by construction: no
// includes, no allocation, no vocabulary, no platform branch. Everything
// any sygaldreye artifact contains; everything else is chosen by a tape.

namespace syg::escapement {

// The ch. 13 calling convention, at the only layer the escapement needs:
// process reads resolved inputs and writes outputs, noexcept, RT-clean.
struct node {
  using process_fn = void (*)(void* state, const float* const* in,
                              float* const* out, int frames) noexcept;
  void* state;
  const float* const* in;   // resolved input port pointers (frozen wiring)
  float* const* out;        // output port pointers
  process_fn process;
};

// A movement: a frozen, flattened, realized graph the escapement ticks.
struct movement {
  node* nodes;
  int count;
};

constexpr void tick(const movement& m, int frames) noexcept {
  for (int i = 0; i < m.count; ++i)
    m.nodes[i].process(m.nodes[i].state, m.nodes[i].in, m.nodes[i].out, frames);
}

}  // namespace syg::escapement
