// clause: fixture (ADR-036) — the ABI-2/3 audit node bodies: a deliberate RT-allocator
// (proves the no-RT-alloc audit bites) and a fault injector. Permanent audit
// fixtures — a violation probe cannot be a well-behaved real native, so the
// CNF-1 "real fallible natives carry these assertions" dissolution a prior
// marker anticipated does not apply.
#include "testnodes/testnodes.hpp"

#include <stdexcept>

#include "phase.hpp"

namespace syg::nodes {

void parsey_body(const float* in, float* out, int frames) {
  // AUT-2 exception: harness fixture
  for (int i = 0; i < frames; ++i) {
    if (in[i] < 0.0f) throw std::runtime_error("malformed input");
    out[i] = in[i];
  }
}

void boom_body(const float* in, float* out, int frames) {
  // AUT-2 exception: harness fixture
  for (int i = 0; i < frames; ++i) {
    if (in[i] > 0.5f) throw std::runtime_error("undeclared trouble");
    out[i] = in[i];
  }
}

void devicey_prepare_body(int, int) { syg::abi::acquire_device("test-dac"); }

void devicey_bad_create_body() { syg::abi::acquire_device("test-dac"); }

}  // namespace syg::nodes
