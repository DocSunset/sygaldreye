// clause: machinery — the reflection seam (core-named): a node seeing its
// enclosing graph through set_context injection only (LNG-7, never a
// global reach). Publishes the ROOT graph's node-key count as a value.
#include "crown.hpp"

#include "native_ports.hpp"
#include "parser/parser.hpp"

namespace syg::organs {
namespace {

struct gs_state {
  const graph_doc* doc = nullptr;
};

void gs_set_context(void* s, const void* ctx) {
  const auto* c = static_cast<const syg::crown::node_context*>(ctx);
  static_cast<gs_state*>(s)->doc = static_cast<const graph_doc*>(c->doc);
}

void gs_value_tick(void* s, double, const float*, float* outs) {
  auto* st = static_cast<gs_state*>(s);
  outs[0] = st->doc ? static_cast<float>(st->doc->nodes.size()) : -1.0f;
}

}  // namespace

extern const syg::crown::native_type graph_source_native;
const syg::crown::native_type graph_source_native{
    "graph_source", [] { return static_cast<void*>(new gs_state()); },
    [](void* s) { delete static_cast<gs_state*>(s); },
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    [](void*, const float* const*, float* const*, int) noexcept {},
    gs_value_tick, syg::generated::graph_source_in_ports(),
    syg::generated::graph_source_out_ports(), false, false, nullptr, nullptr,
    nullptr, nullptr, false, gs_set_context};

}  // namespace syg::organs
