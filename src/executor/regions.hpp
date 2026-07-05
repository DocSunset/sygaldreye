// clause: machinery — executor package (plans, pacing, mappings)
#pragma once
#include <string>
#include <vector>

#include "crown.hpp"
#include "parser/parser.hpp"

namespace syg::executor {

struct boundary {
  std::size_t edge;     // index into the doc's edge list
  std::string mapping;  // from the promise oracle (latch, snapshot, ...)
};

struct region_map {
  std::vector<std::string> block, frame;
  std::vector<boundary> mappings;   // derived; NEVER persisted
  std::vector<std::string> inert;   // no demanded output, no clock (EXE-11.3)
  std::vector<std::string> errors;  // illegal edges, named
};

// Inferred, never declared (EXE-2): recompute from the graph doc and the
// linked registry's port promises.
region_map infer_regions(const organs::graph_doc& g);

// The ONE block schedule (ADR-013): Tarjan SCC + Kahn over the
// condensation, members in doc order. The interpreter's segments and the
// codegen backend both consume THIS — never two orderings (FRZ byte-
// identity hangs on it). Returns ordered components; self_loop flags
// single-node components with a self edge.
struct scc_schedule {
  std::vector<std::vector<std::size_t>> components;
  std::vector<bool> self_loop;  // parallel to components
};
scc_schedule scc_order(std::size_t n,
                       const std::vector<std::pair<std::size_t, std::size_t>>&
                           edges);  // edges pre-cut by the caller

}  // namespace syg::executor
