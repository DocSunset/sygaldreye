// clause: floor — kernel natives over ugens
#include "hello_natives/hello_natives.hpp"

#include <cstring>

#include "ugens/ugens.hpp"

namespace syg::nodes {
namespace {

constexpr float half_pi = 1.57079632679f;

void osc_set_num(void* s, const char* port, double v) {
  if (!std::strcmp(port, "freq")) static_cast<osc_state*>(s)->phasor.freq = float(v);
}
void osc_set_text(void* s, const char* port, const char* v) {
  // shape: a cosine is the sine kernel started a quarter turn in
  if (!std::strcmp(port, "shape") && !std::strcmp(v, "cosine"))
    static_cast<osc_state*>(s)->phasor.phase = half_pi;
}
void lfo_set_num(void* s, const char* port, double v) {
  auto* st = static_cast<lfo_state*>(s);
  if (!std::strcmp(port, "freq")) st->phasor.freq = float(v);
  if (!std::strcmp(port, "depth")) st->depth = float(v);
}
void no_num(void*, const char*, double) {}
void no_text(void*, const char*, const char*) {}

const syg::crown::native_type osc_native{
    "osc",
    [] { return static_cast<void*>(new osc_state{{0.0f, 440.0f, 48000.0f}}); },
    [](void* s) { delete static_cast<osc_state*>(s); },
    osc_set_num, osc_set_text, osc_process, {}, {"out"}};

const syg::crown::native_type lfo_native{
    "lfo",
    [] { return static_cast<void*>(new lfo_state{{0.0f, 440.0f, 48000.0f}, 1.0f}); },
    [](void* s) { delete static_cast<lfo_state*>(s); },
    lfo_set_num, no_text, lfo_process, {}, {"out"}};

const syg::crown::native_type vca_native{
    "vca", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    no_num, no_text, vca_process, {"in", "gain"}, {"out"}};

}  // namespace

extern const syg::crown::native_type dac_native;  // dac_native.cpp

const std::vector<const syg::crown::native_type*>& hello_natives() {
  static const std::vector<const syg::crown::native_type*> all{
      &osc_native, &lfo_native, &vca_native, &dac_native};
  return all;
}

}  // namespace syg::nodes
