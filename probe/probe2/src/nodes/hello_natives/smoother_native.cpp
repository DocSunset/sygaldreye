// clause: floor — a slew-limiting boundary mapping over the salvaged Slew
// kernel: the user-supplied replacement for a default latch (EXE-8, CMP-4)
#include "crown.hpp"

#include <cstring>

#include "native_ports.hpp"
#include "synth_core/kernels.hpp"

namespace syg::nodes {
namespace {

struct smoother_state {
  synth::Slew slew;
  float target = 0.0f;
  float rate = 20.0f;  // units per second
};
void smoother_set(void* s, const char* port, double v) {
  auto* st = static_cast<smoother_state*>(s);
  if (!std::strcmp(port, "in")) st->target = float(v);
  if (!std::strcmp(port, "rate")) st->rate = float(v);
}
void smoother_process(void* s, const float* const*, float* const* out,
                      int frames) noexcept {
  auto* st = static_cast<smoother_state*>(s);
  float max_step = st->rate / 48000.0f;  // dt clamping lives in the shell
  // AUT-2 exception: per-sample slew state, a mapping's floor
  for (int i = 0; i < frames; ++i)
    out[0][i] = st->slew.tick(st->target, max_step);
}

}  // namespace

extern const syg::crown::native_type smoother_native;
const syg::crown::native_type smoother_native{
    "smoother", [] { return static_cast<void*>(new smoother_state()); },
    [](void* s) { delete static_cast<smoother_state*>(s); },
    smoother_set, [](void*, const char*, const char*) {}, smoother_process,
    nullptr, syg::generated::smoother_in_ports(),
    syg::generated::smoother_out_ports(), false, false, nullptr, nullptr,
    nullptr, nullptr, /*is_mapping=*/true};

}  // namespace syg::nodes
