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

// One structured edit op (LNG-5 vocabulary, the arbiter's food — ADR-023).
struct edit_op {
  std::string op;  // set_param | add_node | remove_node | add_edge | remove_edge
  std::string a, b;      // routes / ids / values per op
  std::string author;    // peer key (attribution, LNG-5.2)
  bool undo_replay = false;  // inverse application: cursor move, not history
};

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
  float value_of(const std::string& id_port) const;  // frame cell peek
  long process_calls() const;  // kernel invocations (EXE-10.2 observability)
  long rejected_ops() const { return rejected_; }  // precondition losers
  const std::vector<applied_op>& log() const { return log_; }
  std::size_t log_cursor() const { return cursor_; }
  void undo();  // move the cursor back, applying inverses (ADR-018:
                // the log is append-only; linearity is a view)

 private:
  struct impl;
  static std::unique_ptr<impl> build_impl(const organs::graph_doc&,
                                          const region_map&, int block);
  void apply_structural(const edit_op& o);
  void rebuild();  // re-realize, migrating state by route
  std::unique_ptr<impl> im_;
  std::vector<applied_op> log_;
  std::size_t cursor_ = 0;
  long rejected_ = 0;
  mpsc<edit_op> inlet_q_;
  mpsc<std::pair<std::string, double>> event_q_;
  std::map<std::string, std::string> param_journal_;  // route -> last value
  organs::graph_doc doc_;
  region_map regions_;
  int rate_, block_;
};

}  // namespace syg::executor
