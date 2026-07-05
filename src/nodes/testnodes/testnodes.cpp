// clause: scaffolding (dissolves: CNF-1) — ABI-2/3 audit node bodies
#include "testnodes/testnodes.hpp"

#include <stdexcept>

#include "phase.hpp"

namespace syg::nodes {

void parsey_body(const float* in, float* out, int frames) {
  for (int i = 0; i < frames; ++i) {
    if (in[i] < 0.0f) throw std::runtime_error("malformed input");
    out[i] = in[i];
  }
}

void boom_body(const float* in, float* out, int frames) {
  for (int i = 0; i < frames; ++i) {
    if (in[i] > 0.5f) throw std::runtime_error("undeclared trouble");
    out[i] = in[i];
  }
}

void devicey_prepare_body(int, int) { syg::abi::acquire_device("test-dac"); }

void devicey_bad_create_body() { syg::abi::acquire_device("test-dac"); }

}  // namespace syg::nodes
