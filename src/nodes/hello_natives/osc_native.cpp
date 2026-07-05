// clause: floor — kernel native over ugens (generated registration references
// this TU's symbol; omitting the object is a loud link error — SZ-2)
#include "crown.hpp"

#include "native_ports.hpp"

#include <cstring>

#include "ugens/ugens.hpp"

namespace syg::nodes {
namespace {
constexpr float half_pi = 1.57079632679f;

void osc_set_num(void* s, const char* port, double v) {
  if (!std::strcmp(port, "freq")) static_cast<osc_state*>(s)->phasor.freq = float(v);
}
void osc_set_text(void* s, const char* port, const char* v) {
  // shape: a cosine is the sine read a quarter turn ahead — an offset, so
  // re-applying the default never disturbs migrated phase state (EXE-5)
  if (!std::strcmp(port, "shape") && !std::strcmp(v, "cosine"))
    static_cast<osc_state*>(s)->offset = half_pi;
}
}  // namespace

extern const syg::crown::native_type osc_native;
const syg::crown::native_type osc_native{
    "osc",
    [] { return static_cast<void*>(new osc_state{{0.0f, 440.0f, 48000.0f}, 0.0f}); },
    [](void* s) { delete static_cast<osc_state*>(s); },
    osc_set_num, osc_set_text, osc_process,
    syg::generated::osc_in_ports, syg::generated::osc_out_ports};

}  // namespace syg::nodes
