// clause: floor — white-noise kernel native (synth_core's xorshift)
#include "crown.hpp"

#include "native_ports.hpp"

#include "synth_core/synth_core.hpp"

namespace syg::nodes {
namespace {

struct noise_state {
  uint32_t rng = 0x1234567u;
};

void noise_process(void* s, const float* const*, float* const* out,
                   int frames) noexcept {
  auto& st = *static_cast<noise_state*>(s);
  for (int i = 0; i < frames; ++i) out[0][i] = synth::white_noise(st.rng);
}

}  // namespace

extern const syg::crown::native_type noise_native;
const syg::crown::native_type noise_native{
    "noise", [] { return static_cast<void*>(new noise_state()); },
    [](void* s) { delete static_cast<noise_state*>(s); },
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    noise_process, nullptr, syg::generated::noise_in_ports(), syg::generated::noise_out_ports()};

}  // namespace syg::nodes
