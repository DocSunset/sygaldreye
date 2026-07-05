#pragma once
#include <map>
#include <string_view>
#include <memory>
#include <string>
#include <vector>

#include "escapement.hpp"

namespace syg::crown {

// The structured lane (ADR-034): a kind-tagged payload riding the event
// and value disciplines. Zero-copy in-process — the pointer is shared,
// the value immutable; canonical encoding happens only at commit
// boundaries (ADR-017). Generated accessors (svalue_accessors.hpp) are
// the ONLY sanctioned cast: C++ type <-> kind by generated declaration.
struct svalue {
  const char* kind = "";                 // catalog kind name
  std::shared_ptr<const void> value;     // the in-motion form
};

// One structured edit op (LNG-5, ADR-023): what every arbiter inlet eats.
struct edit_op {
  std::string op;  // set_param | add_node | remove_node | add_edge | remove_edge
  std::string a, b;
  std::string author;        // peer key (attribution)
  bool undo_replay = false;  // inverse application: cursor move, not history
};

// What set_context delivers (the LNG-7 seam): the enclosing graph and an
// arbiter submit hook — WHICH plan's arbiter is a wiring choice, which is
// how a graph edits a graph (LNG-11.3).
struct node_context {
  const void* doc = nullptr;  // organs::graph_doc*
  void (*submit)(void* plan, edit_op op) = nullptr;
  void* plan = nullptr;
};

// mirrors the catalog's non-float set (ADR-034): these ride event/value
// only; the oracle refuses them on stream
inline bool structured_kind(const char* k) {
  const std::string_view v(k);
  return v == "graph" || v == "ops" || v == "text" || v == "mesh" ||
         v == "texture" || v == "surface" || v == "draw_call" ||
         v == "cidset";
}

// A port declaration with its promises (generated from the endpoints
// struct — AUT-3; the crown reads names, the executor reads promises).
struct port_decl {
  const char* name;
  const char* kind;
  const char* discipline;
};

// A linked native: what the registration TU provides. The crown never
// looks inside; it only instantiates, wires, and parameterizes.
struct native_type {
  const char* name;
  void* (*create)();
  void (*destroy)(void*);
  void (*set_num)(void*, const char* port, double v);
  void (*set_text)(void*, const char* port, const char* v);
  escapement::node::process_fn process;
  // value-discipline behavior (frame side): recompute outs from ins after
  // dt; null for pure stream natives
  void (*value_tick)(void* state, double dt, const float* ins, float* outs);
  std::vector<port_decl> in_ports, out_ports;
  // time-dependent: wired to its executor's clock (ADR-015 — the visible
  // clock-input wiring arrives with the engine graph; this flag stands in)
  bool clocked = false;
  // whole-block interior (FFT-shaped): a cycle through it demands an
  // explicit delay at edit time (ADR-013)
  bool block_override = false;
  // the event applier hook (ch. 13): consume events, write state; runs
  // before process at the consumer's boundary, never mid-tick
  void (*apply)(void* state, const char* port, double v) = nullptr;
  // the structured lane's hooks (ADR-034, LNG-11): value-discipline
  // recompute over kind-tagged payloads; structured event applier; pull
  // hook for event emission (polled after appliers, same boundary)
  void (*svalue_tick)(void* state, const svalue* ins, svalue* outs) = nullptr;
  void (*sapply)(void* state, const char* port, const svalue& v) = nullptr;
  bool (*semit)(void* state, const char* port, svalue* out) = nullptr;
  // a mapping node: its ports are throughpoints; compilation inserts no
  // default mapping where one already sits (L13)
  bool is_mapping = false;
  // the context seam (ch. 13, LNG-7): resource holders and reflection
  // receive their context by injection at pump time — never global reach
  void (*set_context)(void* state, const void* ctx) = nullptr;
  // owns a platform resource: refuses to lift (LNG-6.2, AUT-4)
  bool resource_holder = false;
};

// One op record (the five appliers; the tape and every editor speak this).
struct op {
  enum class kind { instantiate, link, unlink, remove, write_default } what;
  std::string a, b, c;  // instantiate: id,type · link/unlink: from,to ·
                        // remove: id · write_default: route,value
};

class plan {
 public:
  explicit plan(std::vector<const native_type*> linked, int block);
  plan(plan&&) noexcept;
  ~plan();

  void submit(op o);        // the op inlet
  void tick(int frames);    // drain inlet at the boundary, then process

  // introspection (harness + serialization faces)
  struct record { std::string id, type; };
  const std::vector<record>& nodes() const { return order_; }
  const std::vector<std::pair<std::string, std::string>>& edges() const {
    return edges_;
  }
  const std::vector<std::pair<std::string, std::string>>& defaults() const {
    return defaults_;
  }
  const float* input_buffer(const std::string& id, const std::string& port) const;

  // state surgery for the slot organ (EXE-5): exchange an instance's state
  // pointer; the caller owns what comes back
  void* exchange_state(const std::string& id, void* replacement);
  const native_type* type_of(const std::string& id) const;

 private:
  struct instance;
  void apply(const op& o);
  void rewire();

  std::vector<const native_type*> linked_;
  int block_;
  std::vector<record> order_;
  std::map<std::string, std::unique_ptr<instance>> instances_;
  std::vector<std::pair<std::string, std::string>> edges_;
  std::vector<std::pair<std::string, std::string>> defaults_;
  std::vector<op> inlet_;
  std::vector<escapement::node> ticklist_;
  std::vector<float> silence_;
};

// The tape reader (FMT-3): pinned record grammar -> ops. Throws on a bad
// record, naming the line.
std::vector<op> read_tape(const std::string& text);

}  // namespace syg::crown
