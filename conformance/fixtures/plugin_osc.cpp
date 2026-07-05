// The SHIPPED PLUGIN route (AUT-5.1c): an osc authored outside the tree,
// compiled to a .so, loaded through the plugin gate — semantics copied
// exactly from the native osc so the golden comparison is byte-strength.
#include <cmath>
#include <cstring>

#include "crown.hpp"

namespace {

constexpr float k2pi = 6.28318530f;
constexpr float half_pi = 1.57079632679f;

struct state {
  float phase = 0.0f, freq = 440.0f, offset = 0.0f;
};

void set_num(void* s, const char* port, double v) {
  if (!std::strcmp(port, "freq")) static_cast<state*>(s)->freq = float(v);
}
void set_text(void* s, const char* port, const char* v) {
  if (!std::strcmp(port, "shape") && !std::strcmp(v, "cosine"))
    static_cast<state*>(s)->offset = half_pi;
}
void process(void* sv, const float* const*, float* const* out,
             int frames) noexcept {
  auto& s = *static_cast<state*>(sv);
  for (int i = 0; i < frames; ++i) {
    out[0][i] = std::sin(s.phase + s.offset);
    s.phase += k2pi * s.freq / 48000.0f;
    if (s.phase >= k2pi) s.phase -= k2pi;
  }
}

const syg::crown::native_type plugin_type{
    "plugin_osc",
    [] { return static_cast<void*>(new state()); },
    [](void* s) { delete static_cast<state*>(s); },
    set_num, set_text, process, nullptr,
    {{"freq", "scalar", "value"}, {"shape", "text", "value"}},
    {{"out", "audio", "block"}}};

}  // namespace

extern "C" const syg::crown::native_type* syg_plugin_native() {
  return &plugin_type;
}
