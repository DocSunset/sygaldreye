// clause: floor — the feedback-loop natives: delay (DelayLine kernel,
// salvaged verbatim), pulse (unit impulse), spectro (a block-override
// stand-in: consumes its block whole)
#include "crown.hpp"

#include <cstring>

#include "native_ports.hpp"
#include "synth_core/kernels.hpp"

namespace syg::nodes {
namespace {

struct delay_state {
  synth::DelayLine line;
  int samples = 1;
};
void delay_set(void* s, const char* port, double v) {
  auto* st = static_cast<delay_state*>(s);
  if (!std::strcmp(port, "samples")) {
    st->samples = static_cast<int>(v);
    st->line.prepare(st->samples + 1);  // boundary-time sizing (L9)
  }
}
void delay_process(void* s, const float* const* in, float* const* out,
                   int frames) noexcept {
  auto* st = static_cast<delay_state*>(s);
  // AUT-2 exception: loop-discipline fixture
  for (int i = 0; i < frames; ++i)
    out[0][i] = st->line.tick(in[0][i], st->samples, 0.0f);
}

struct pulse_state {
  bool fired = false;
};
void pulse_process(void* s, const float* const*, float* const* out,
                   int frames) noexcept {
  auto* st = static_cast<pulse_state*>(s);
  // AUT-2 exception: loop-discipline fixture
  for (int i = 0; i < frames; ++i) out[0][i] = 0.0f;
  if (!st->fired && frames > 0) {
    out[0][0] = 1.0f;
    st->fired = true;
  }
}

void spectro_process(void*, const float* const* in, float* const* out,
                      int frames) noexcept {
  // AUT-2 exception: loop-discipline fixture
  for (int i = 0; i < frames; ++i) out[0][i] = in[0][i];  // passthrough tap
}

}  // namespace

extern const syg::crown::native_type delay_native;
const syg::crown::native_type delay_native{
    "delay", [] { return static_cast<void*>(new delay_state()); },
    [](void* s) { delete static_cast<delay_state*>(s); },
    delay_set, [](void*, const char*, const char*) {}, delay_process, nullptr,
    syg::generated::delay_in_ports(), syg::generated::delay_out_ports()};

extern const syg::crown::native_type pulse_native;
const syg::crown::native_type pulse_native{
    "pulse", [] { return static_cast<void*>(new pulse_state()); },
    [](void* s) { delete static_cast<pulse_state*>(s); },
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    pulse_process, nullptr,
    syg::generated::pulse_in_ports(), syg::generated::pulse_out_ports()};

extern const syg::crown::native_type spectro_native;
const syg::crown::native_type spectro_native{
    "spectro", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    spectro_process, nullptr,
    syg::generated::spectro_in_ports(), syg::generated::spectro_out_ports(),
    false, true};  // clocked=false, block_override=true

}  // namespace syg::nodes
