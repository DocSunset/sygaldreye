// clause: machinery — op_button: a gesture-shaped source turning a bang
// into an edit-op event (the editor's gesture nodes will be subgraphs
// over this shape)
#include <cstring>

#include <nlohmann/json.hpp>

#include "crown.hpp"
#include "native_ports.hpp"
#include "svalue_accessors.hpp"

namespace syg::nodes {
namespace {

struct opb_state {
  syg::crown::edit_op op;
  bool pending = false;
};
void opb_set_text(void* s, const char* port, const char* v) {
  if (std::strcmp(port, "op")) return;
  auto j = nlohmann::json::parse(v);
  static_cast<opb_state*>(s)->op = {j.value("op", "set_param"),
                                    j.value("a", j.value("route", "")),
                                    j.value("b", j.value("value", "")),
                                    j.value("author", "op_button")};
}
void opb_apply(void* s, const char* port, double) {
  if (!std::strcmp(port, "in")) static_cast<opb_state*>(s)->pending = true;
}
bool opb_semit(void* s, const char* port, syg::crown::svalue* out) {
  auto* st = static_cast<opb_state*>(s);
  if (std::strcmp(port, "out") || !st->pending) return false;
  st->pending = false;
  *out = syg::generated::make_ops({st->op});
  return true;
}
void no_process(void*, const float* const*, float* const*, int) noexcept {}

}  // namespace

extern const syg::crown::native_type op_button_native;
const syg::crown::native_type op_button_native{
    "op_button", [] { return static_cast<void*>(new opb_state()); },
    [](void* s) { delete static_cast<opb_state*>(s); },
    [](void*, const char*, double) {}, opb_set_text, no_process, nullptr,
    {{"in", "bang", "event"}, {"op", "text", "value"}},
    {{"out", "ops", "event"}},
    false, false, opb_apply, nullptr, nullptr, opb_semit};

}  // namespace syg::nodes
