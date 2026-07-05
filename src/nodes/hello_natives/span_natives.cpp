// clause: floor — the span vocabulary: spanv (a span-valued source; its
// list default drives lift expansion), mix (whole-by-kind N-ary sum), and
// instanced_draw (a headless stand-in for the render boundary consuming a
// span whole — the GL reality arrives with the render package, rung 8;
// scaffolding-adjacent but the SEMANTICS here are permanent)
#include "crown.hpp"

#include <cstring>

#include "native_ports.hpp"

namespace syg::nodes {
namespace {

struct n_state {
  int n = 0;
};
void n_set(void* s, const char* port, double v) {
  if (!std::strcmp(port, "n")) static_cast<n_state*>(s)->n = int(v);
}
void mix_process(void* s, const float* const* in, float* const* out,
                 int frames) noexcept {
  auto* st = static_cast<n_state*>(s);
  for (int i = 0; i < frames; ++i) {
    float acc = 0.0f;
    for (int k = 0; k < st->n; ++k) acc += in[k][i];
    out[0][i] = acc;
  }
}

struct draw_state {
  int n = 0;
  long calls = 0;
};
void draw_set(void* s, const char* port, double v) {
  if (!std::strcmp(port, "n")) static_cast<draw_state*>(s)->n = int(v);
}
void draw_value_tick(void* s, double, const float*, float* outs) {
  auto* st = static_cast<draw_state*>(s);
  ++st->calls;  // ONE call per present, however many instances
  outs[0] = static_cast<float>(st->calls);
  outs[1] = static_cast<float>(st->n);
}
void no_process(void*, const float* const*, float* const*, int) noexcept {}

}  // namespace

extern const syg::crown::native_type spanv_native;
const syg::crown::native_type spanv_native{
    "spanv", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    no_process, nullptr,
    syg::generated::spanv_in_ports(), syg::generated::spanv_out_ports()};

extern const syg::crown::native_type mix_native;
const syg::crown::native_type mix_native{
    "mix", [] { return static_cast<void*>(new n_state()); },
    [](void* s) { delete static_cast<n_state*>(s); },
    n_set, [](void*, const char*, const char*) {}, mix_process, nullptr,
    syg::generated::mix_in_ports(), syg::generated::mix_out_ports()};

extern const syg::crown::native_type instanced_draw_native;
const syg::crown::native_type instanced_draw_native{
    "instanced_draw", [] { return static_cast<void*>(new draw_state()); },
    [](void* s) { delete static_cast<draw_state*>(s); },
    draw_set, [](void*, const char*, const char*) {}, no_process,
    draw_value_tick, syg::generated::instanced_draw_in_ports(),
    syg::generated::instanced_draw_out_ports(), true};  // presents per frame

}  // namespace syg::nodes
