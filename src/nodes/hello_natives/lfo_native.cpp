// clause: floor — kernel native over ugens (generated registration references
// this TU's symbol; omitting the object is a loud link error — SZ-2)
#include "crown.hpp"

#include <cstring>

#include "ugens/ugens.hpp"

namespace syg::nodes {
namespace {
void lfo_set_num(void* s, const char* port, double v) {
  auto* st = static_cast<lfo_state*>(s);
  if (!std::strcmp(port, "freq")) st->phasor.freq = float(v);
  if (!std::strcmp(port, "depth")) st->depth = float(v);
}
void lfo_no_text(void*, const char*, const char*) {}
}  // namespace

extern const syg::crown::native_type lfo_native;
const syg::crown::native_type lfo_native{
    "lfo",
    [] { return static_cast<void*>(new lfo_state{{0.0f, 440.0f, 48000.0f}, 1.0f}); },
    [](void* s) { delete static_cast<lfo_state*>(s); },
    lfo_set_num, lfo_no_text, lfo_process, {}, {"out"}};

}  // namespace syg::nodes
