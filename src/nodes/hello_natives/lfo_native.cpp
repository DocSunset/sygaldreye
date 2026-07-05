// clause: floor — kernel native over ugens (generated registration references
// this TU's symbol; omitting the object is a loud link error — SZ-2)
#include "crown.hpp"

#include "native_ports.hpp"

#include <cstring>

#include "synth_core/synth_core.hpp"
#include "ugens/ugens.hpp"

namespace syg::nodes {
namespace {
void lfo_set_num(void* s, const char* port, double v) {
  auto* st = static_cast<lfo_state*>(s);
  if (!std::strcmp(port, "freq")) st->phasor.freq = float(v);
  if (!std::strcmp(port, "depth")) st->depth = float(v);
}
void lfo_no_text(void*, const char*, const char*) {}

// the value-discipline face: one unipolar sample per demand (frame clock)
void lfo_value_tick(void* s, double dt, const float*, float* outs) {
  auto* st = static_cast<lfo_state*>(s);
  st->phasor.phase += 6.28318530f * st->phasor.freq * static_cast<float>(dt);
  if (st->phasor.phase >= 6.28318530f) st->phasor.phase -= 6.28318530f;
  outs[0] = st->depth * 0.5f * (1.0f + synth::sine(st->phasor.phase));
}
}  // namespace

extern const syg::crown::native_type lfo_native;
const syg::crown::native_type lfo_native{
    "lfo",
    [] { return static_cast<void*>(new lfo_state{{0.0f, 440.0f, 48000.0f}, 1.0f}); },
    [](void* s) { delete static_cast<lfo_state*>(s); },
    lfo_set_num, lfo_no_text, lfo_process, lfo_value_tick,
    syg::generated::lfo_in_ports, syg::generated::lfo_out_ports, true};

}  // namespace syg::nodes
