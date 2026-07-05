#include "hello_cosine.hpp"

#include "escapement.hpp"
#include "ugens/ugens.hpp"

namespace syg::movements {

// The fixture graph, frozen by hand: four nodes, three edges, defaults
// osc0/freq=220 shape=cosine, lfo0/freq=0.5. The dac is the sink boundary.
void render_hello_cosine(int frames,
                         void (*sink)(void* ctx, const float* block, int n),
                         void* ctx) {
  constexpr int block = hello_cosine_block;
  constexpr float rate = static_cast<float>(hello_cosine_rate);
  float osc_out[block], lfo_out[block], vca_out[block];

  nodes::osc_state osc{{0.0f, 220.0f, rate}};
  nodes::lfo_state lfo{{0.0f, 0.5f, rate}, 1.0f};

  float* osc_outs[] = {osc_out};
  float* lfo_outs[] = {lfo_out};
  const float* vca_ins[] = {osc_out, lfo_out};  // in, gain
  float* vca_outs[] = {vca_out};

  escapement::node plan[] = {
      {&osc, nullptr, osc_outs, nodes::osc_process},
      {&lfo, nullptr, lfo_outs, nodes::lfo_process},
      {nullptr, vca_ins, vca_outs, nodes::vca_process},
  };
  escapement::movement m{plan, 3};

  for (int done = 0; done < frames; done += block) {
    int n = frames - done < block ? frames - done : block;
    escapement::tick(m, n);
    sink(ctx, vca_out, n);
  }
}

}  // namespace syg::movements
