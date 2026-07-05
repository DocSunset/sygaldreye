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
  std::vector<std::string> errors;  // illegal edges, named
};

// Inferred, never declared (EXE-2): recompute from the graph doc and the
// linked registry's port promises.
region_map infer_regions(const organs::graph_doc& g);

}  // namespace syg::executor
