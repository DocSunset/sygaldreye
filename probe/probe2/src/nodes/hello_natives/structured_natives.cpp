// clause: machinery — the structured lane's starter vocabulary: graph_cell
// (a graph-valued source loading a dataset by name — dataset IO is the
// world), node_count (a pure structured transform; an AUT-3 one-liner once
// the stamp covers the lane), text_cell (a settable text source)
#include "crown.hpp"

#include <cstring>
#include <fstream>

#include "native_ports.hpp"
#include "parser/parser.hpp"
#include "svalue_accessors.hpp"

namespace syg::nodes {
namespace {

namespace gen = syg::generated;

struct gcell_state {
  syg::crown::svalue held;
};
void gcell_set_text(void* s, const char* port, const char* v) {
  if (std::strcmp(port, "name")) return;
  std::ifstream f(std::string("graphs/") + v + ".json");
  if (!f) return;  // boundary-time load; a miss leaves the cell empty
  static_cast<gcell_state*>(s)->held =
      gen::make_graph(syg::organs::parse_graph(nlohmann::json::parse(f)));
}
void gcell_svalue_tick(void* s, const syg::crown::svalue*,
                       syg::crown::svalue* outs) {
  outs[0] = static_cast<gcell_state*>(s)->held;  // zero-copy: shared value
}

void node_count_svalue_tick(void*, const syg::crown::svalue* ins,
                            syg::crown::svalue* outs) {
  if (!ins[0].value) return;
  outs[0] = gen::make_text(
      std::to_string(gen::as_graph(ins[0]).nodes.size()));
}

struct tcell_state {
  syg::crown::svalue held = gen::make_text("");
};
void tcell_set_text(void* s, const char* port, const char* v) {
  if (!std::strcmp(port, "value"))
    static_cast<tcell_state*>(s)->held = gen::make_text(v);
}
void tcell_svalue_tick(void* s, const syg::crown::svalue*,
                       syg::crown::svalue* outs) {
  outs[0] = static_cast<tcell_state*>(s)->held;
}
void no_process(void*, const float* const*, float* const*, int) noexcept {}

}  // namespace

extern const syg::crown::native_type graph_cell_native;
const syg::crown::native_type graph_cell_native{
    "graph_cell", [] { return static_cast<void*>(new gcell_state()); },
    [](void* s) { delete static_cast<gcell_state*>(s); },
    [](void*, const char*, double) {}, gcell_set_text, no_process, nullptr,
    syg::generated::graph_cell_in_ports(),
    syg::generated::graph_cell_out_ports(), false, false, nullptr,
    gcell_svalue_tick};

extern const syg::crown::native_type node_count_native;
const syg::crown::native_type node_count_native{
    "node_count", [] { return static_cast<void*>(nullptr); }, [](void*) {},
    [](void*, const char*, double) {}, [](void*, const char*, const char*) {},
    no_process, nullptr,
    syg::generated::node_count_in_ports(),
    syg::generated::node_count_out_ports(), false, false, nullptr,
    node_count_svalue_tick};

extern const syg::crown::native_type text_cell_native;
const syg::crown::native_type text_cell_native{
    "text_cell", [] { return static_cast<void*>(new tcell_state()); },
    [](void* s) { delete static_cast<tcell_state*>(s); },
    [](void*, const char*, double) {}, tcell_set_text, no_process, nullptr,
    syg::generated::text_cell_in_ports(),
    syg::generated::text_cell_out_ports(), false, false, nullptr,
    tcell_svalue_tick};

}  // namespace syg::nodes
