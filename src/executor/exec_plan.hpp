#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "crown.hpp"
#include "parser/parser.hpp"
#include "regions.hpp"

namespace syg::executor {

// One structured edit op (LNG-5 vocabulary, the arbiter's food — ADR-023).
struct edit_op {
  std::string op;  // set_param | add_node | remove_node | add_edge | remove_edge
  std::string a, b;      // routes / ids / values per op
  std::string author;    // peer key (attribution, LNG-5.2)
};

// The compiled runtime cache of a graph (EXE-1): the block region as fused
// segments over plan-owned buffers, the frame region as value cells with
// dirty bits (ADR-015), latches at the inferred boundaries. One op inlet;
// ops apply at block boundaries only.
class exec_plan {
 public:
  exec_plan(organs::graph_doc doc, int rate, int block);
  ~exec_plan();
  exec_plan(exec_plan&&) noexcept;

  void submit(edit_op o);

  // one block: boundary (drain inlet) -> frame tick if due -> latch ->
  // block segments. Returns the sink's samples (dac input).
  const float* pump_block();

  int rate() const { return rate_; }
  int block() const { return block_; }
  const organs::graph_doc& doc() const { return doc_; }
  const region_map& regions() const { return regions_; }
  long recomputes(const std::string& id) const;  // EXE-11 counters
  float value_of(const std::string& id_port) const;  // frame cell peek

 private:
  struct impl;
  std::unique_ptr<impl> im_;
  organs::graph_doc doc_;
  region_map regions_;
  int rate_, block_;
};

}  // namespace syg::executor
