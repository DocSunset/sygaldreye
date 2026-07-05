// clause: floor — value-discipline leaf natives (cell: a settable source;
// scale: out = in * k). AUT-3 one-liners once the stamp exists.
#include "crown.hpp"

#include <cstring>

#include "native_ports.hpp"

namespace syg::nodes {
namespace {

struct knob {
  float k = 0.0f;
};
void knob_set(void* s, const char* port, double v) {
  if (!std::strcmp(port, "k")) static_cast<knob*>(s)->k = float(v);
}
void no_text(void*, const char*, const char*) {}
void no_process(void*, const float* const*, float* const*, int) noexcept {}

void cell_value_tick(void* s, double, const float*, float* outs) {
  outs[0] = static_cast<knob*>(s)->k;
}
void scale_value_tick(void* s, double, const float* ins, float* outs) {
  outs[0] = ins[0] * static_cast<knob*>(s)->k;
}

}  // namespace

extern const syg::crown::native_type cell_native;
const syg::crown::native_type cell_native{
    "cell", [] { return static_cast<void*>(new knob()); },
    [](void* s) { delete static_cast<knob*>(s); },
    knob_set, no_text, no_process, cell_value_tick,
    syg::generated::cell_in_ports(), syg::generated::cell_out_ports()};

extern const syg::crown::native_type scale_native;
const syg::crown::native_type scale_native{
    "scale", [] { return static_cast<void*>(new knob()); },
    [](void* s) { delete static_cast<knob*>(s); },
    knob_set, no_text, no_process, scale_value_tick,
    syg::generated::scale_in_ports(), syg::generated::scale_out_ports()};

namespace {
void tmux_value_tick(void*, double, const float* ins, float* outs) {
  outs[0] = ins[0];  // passthrough; the host binding is rung-9+ work
}
void no_num(void*, const char*, double) {}
}  // namespace

extern const syg::crown::native_type tmux_native;
const syg::crown::native_type tmux_native{
    "tmux", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    no_num, no_text, no_process, tmux_value_tick,
    syg::generated::tmux_in_ports(), syg::generated::tmux_out_ports()};

}  // namespace syg::nodes

