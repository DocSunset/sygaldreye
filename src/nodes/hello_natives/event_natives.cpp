// clause: floor — the event-discipline starter pair: button (a bang
// source; the gesture path posts through its out port) and counter
// (applies bangs to state; exposes count and order-violations as values)
#include "crown.hpp"

#include <cstring>

#include "native_ports.hpp"

namespace syg::nodes {
namespace {

struct counter_state {
  long count = 0;
  double last = 0;
  long disorder = 0;
};
void counter_apply(void* s, const char* port, double v) {
  auto* st = static_cast<counter_state*>(s);
  if (std::strcmp(port, "in")) return;
  ++st->count;
  if (v && v != st->last + 1) ++st->disorder;  // payloads 1..N certify order
  if (v) st->last = v;
}
void counter_value_tick(void* s, double, const float*, float* outs) {
  auto* st = static_cast<counter_state*>(s);
  outs[0] = static_cast<float>(st->count);
  outs[1] = static_cast<float>(st->disorder);
}
void no_process(void*, const float* const*, float* const*, int) noexcept {}

}  // namespace

extern const syg::crown::native_type button_native;
const syg::crown::native_type button_native{
    "button", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    no_process, nullptr,
    syg::generated::button_in_ports(), syg::generated::button_out_ports()};

extern const syg::crown::native_type counter_native;
const syg::crown::native_type counter_native{
    "counter", [] { return static_cast<void*>(new counter_state()); },
    [](void* s) { delete static_cast<counter_state*>(s); },
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    no_process, counter_value_tick,
    syg::generated::counter_in_ports(), syg::generated::counter_out_ports(),
    false, false, counter_apply};

}  // namespace syg::nodes
