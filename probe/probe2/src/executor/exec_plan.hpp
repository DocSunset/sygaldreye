// clause: machinery — executor package (plans, pacing, mappings)
#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "crown.hpp"
#include "parser/parser.hpp"
#include "mpsc.hpp"
#include "regions.hpp"

namespace syg::executor {

struct render_target;  // render_target.hpp — the frame region's device seam

using edit_op = crown::edit_op;  // the arbiter's food (LNG-5, ADR-023)

// The log entry: every applied op carries its inverse (ADR-018).
struct applied_op {
  edit_op op;
  std::vector<edit_op> inverse;
};

// The compiled runtime cache of a graph (EXE-1): the block region as fused
// segments over plan-owned buffers, the frame region as value cells with
// dirty bits (ADR-015), latches at the inferred boundaries. One op inlet;
// ops apply at block boundaries only.
class exec_plan {
 public:
  exec_plan(organs::graph_doc doc, int rate, int block);
  ~exec_plan();

  void submit(edit_op o);            // any thread (the arbiter's inlet)
  void post_event(const std::string& out_route, double v);  // any thread

  // one block: boundary (drain inlet) -> frame tick if due -> latch ->
  // block segments. Returns the sink's samples (dac input).
  const float* pump_block();

  int rate() const { return rate_; }
  int block() const { return block_; }
  const organs::graph_doc& doc() const { return doc_; }
  const region_map& regions() const { return regions_; }
  long recomputes(const std::string& id) const;  // EXE-11 counters
  std::vector<std::string> instance_ids() const;  // realized frame instances
  float value_of(const std::string& id_port) const;  // frame cell peek
  const crown::svalue* svalue_of(const std::string& id_port) const;
  // aim an arbiter_inlet instance at ANOTHER plan's queue (LNG-11.3)
  void point_arbiter(const std::string& id, exec_plan& target);
  void set_store(const void* store);  // inject the store into the seam
  // aim the frame region at a device (PKG-4): at each head-chain completion
  // the executor hands each on-chain draw's held mesh+surface here, in
  // head-chain order. Null (the default) = headless with no presentation.
  void set_render_target(render_target* rt);
  // the executor's own trace of the last frame tick (instance ids, in
  // execution order) — CMP-3.2's witness, never a self-report
  const std::vector<std::string>& last_tick_order() const;
  // executor-side census (CMP-6.1's witness, never a session's hand
  // tally): plans whose doc realizes the engine pipeline (a `realize`
  // instance) are engine levels
  static long live_engine_plans();
  long process_calls() const;  // kernel invocations (EXE-10.2 observability)
  long rejected_ops() const { return rejected_; }  // precondition losers
  const std::vector<std::string>& faults() const { return faults_; }
  const std::vector<applied_op>& log() const { return log_; }
  std::size_t log_cursor() const { return cursor_; }
  void undo();  // move the cursor back one op, applying inverses (ADR-018:
                // the log is append-only; linearity is a view)
  // Structural-snapshot undo (EDR-3): revert back to the previous structural
  // change, folding any param drift in between into the same step ("param
  // drift never trashes history"). One editor undo = one structural snapshot.
  void undo_gesture();

 private:
  struct impl;
  static std::unique_ptr<impl> build_impl(const organs::graph_doc&,
                                          const region_map&, int block);
  void inject_context();
  void apply_structural(const edit_op& o);
  void rebuild();  // re-realize, migrating state by route
  std::unique_ptr<impl> im_;
  crown::node_context ctx_, target_ctx_;
  std::vector<applied_op> log_;
  std::size_t cursor_ = 0;
  long rejected_ = 0;
  std::vector<std::string> faults_;
  int consecutive_overruns_ = 0;
  bool engine_plan_ = false;
  render_target* render_target_ = nullptr;
  void update_census();
  mpsc<edit_op> inlet_q_;
  mpsc<std::pair<std::string, double>> event_q_;
  std::map<std::string, std::pair<std::string, bool>> param_journal_;  // route -> (value, is_text)
  organs::graph_doc doc_;       // the app graph (persisted surface)
  organs::graph_doc expanded_;  // subgraphs cloned open (derived)
  region_map regions_;
  int rate_, block_;
};

}  // namespace syg::executor
