// clause: floor — kernel-wrapping process hooks (AUT-1 wrappers)
#include "ugens.hpp"

namespace syg::nodes {

void osc_process(void* state, const float* const*, float* const* out,
                 int frames) noexcept {
  auto& s = *static_cast<osc_state*>(state);
  for (int i = 0; i < frames; ++i) out[0][i] = synth::sine(s.phasor.tick());
}

void lfo_process(void* state, const float* const*, float* const* out,
                 int frames) noexcept {
  auto& s = *static_cast<lfo_state*>(state);
  for (int i = 0; i < frames; ++i)
    out[0][i] = s.depth * 0.5f * (1.0f + synth::sine(s.phasor.tick()));
}

void vca_process(void*, const float* const* in, float* const* out,
                 int frames) noexcept {
  for (int i = 0; i < frames; ++i) out[0][i] = in[0][i] * in[1][i];
}

}  // namespace syg::nodes
