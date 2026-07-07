#include "ks.hpp"

#include "synth_core/kernels.hpp"

namespace syg::movements {

// The Karplus-Strong island, frozen BY HAND to loop-carried variables —
// exactly the fused form ADR-013 promises the freezer will emit. Must
// stay observationally identical to the interpreted island (EXE-10.4).
void render_ks(int frames, void (*sink)(void*, const float*, int), void* ctx) {
  constexpr int block = 128, N = 99;
  constexpr float gain = 0.995f;
  synth::DelayLine line;
  line.prepare(N + 1);
  float prev_fb = 0.0f;
  bool fired = false;
  float out[block];
  for (int done = 0; done < frames; done += block) {
    int n = frames - done < block ? frames - done : block;
    for (int i = 0; i < n; ++i) {
      float pulse = !fired && done == 0 && i == 0 ? 1.0f : 0.0f;
      if (pulse > 0.0f) fired = true;
      float a = pulse + prev_fb;          // add (z⁻¹ on the feedback arm)
      float d = line.tick(a, N, 0.0f);    // delay
      prev_fb = d * gain;                 // fb vca -> next sample's add
      out[i] = a;                         // the tap
    }
    sink(ctx, out, n);
  }
}

}  // namespace syg::movements
