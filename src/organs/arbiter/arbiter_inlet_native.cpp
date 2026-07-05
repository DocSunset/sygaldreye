// clause: machinery — the arbiter's mouth as a node (ADR-023/034): ops
// arriving as event payloads submit into the contexted plan's one queue.
// Which plan is a wiring/injection choice — no privileged edit surface.
#include <cstring>

#include "crown.hpp"
#include "native_ports.hpp"
#include "svalue_accessors.hpp"

namespace syg::organs {
namespace {

struct arb_state {
  syg::crown::node_context ctx;
};
void arb_set_context(void* s, const void* ctx) {
  static_cast<arb_state*>(s)->ctx =
      *static_cast<const syg::crown::node_context*>(ctx);
}
void arb_sapply(void* s, const char* port, const syg::crown::svalue& v) {
  if (std::strcmp(port, "in")) return;
  auto* st = static_cast<arb_state*>(s);
  if (!st->ctx.submit || std::string_view(v.kind) != "ops") return;
  for (const auto& op : syg::generated::as_ops(v))
    st->ctx.submit(st->ctx.plan, op);
}
void no_process(void*, const float* const*, float* const*, int) noexcept {}

}  // namespace

extern const syg::crown::native_type arbiter_inlet_native;
const syg::crown::native_type arbiter_inlet_native{
    "arbiter_inlet", [] { return static_cast<void*>(new arb_state()); },
    [](void* s) { delete static_cast<arb_state*>(s); },
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    no_process, nullptr, {{"in", "ops", "event"}}, {},
    false, false, nullptr, nullptr, arb_sapply, nullptr, false,
    arb_set_context};

}  // namespace syg::organs
