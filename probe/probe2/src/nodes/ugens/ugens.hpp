// clause: floor — kernel-wrapping process hooks (AUT-1 wrappers; generated
// one-liners once the AUT-3 stamp exists)
#pragma once
#include "escapement.hpp"
#include "synth_core/synth_core.hpp"

namespace syg::nodes {

// The first natives: process hooks over the salvaged kernels, speaking the
// escapement's calling convention. Params are plain state members written
// before the loop (latched per block — the kernel contract).

struct osc_state {
  synth::Phasor phasor;      // freq/sample_rate set at freeze time
  float offset = 0.0f;       // waveform phase offset (cosine = quarter turn)
};
void osc_process(void* state, const float* const* in, float* const* out,
                 int frames) noexcept;

struct lfo_state {
  synth::Phasor phasor;
  float depth = 1.0f;
};
// Unipolar waving in [0, depth]: gain-shaped, so an amplitude waved by an
// lfo breathes at the lfo's own frequency.
void lfo_process(void* state, const float* const* in, float* const* out,
                 int frames) noexcept;

// vca: out = in * gain, per sample.
void vca_process(void* state, const float* const* in, float* const* out,
                 int frames) noexcept;

}  // namespace syg::nodes
