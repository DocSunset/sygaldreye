#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "escapement.hpp"

namespace syg::crown {

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
