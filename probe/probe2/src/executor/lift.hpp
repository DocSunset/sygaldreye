// clause: machinery — executor package (plans, pacing, mappings)
#pragma once
#include "parser/parser.hpp"
#include "subgraph/subgraph.hpp"

namespace syg::executor {

// Lift expansion (LNG-2, AUT-4): an excess-rank edge — a span producer
// wired to a cell-kind consumer — stamps N clones keyed by index
// (`id#k`); span-kind consumers take the span WHOLE (no clones). Runs on
// the doc BEFORE subgraph expansion so template consumers can refuse:
// resource holders never lift (LNG-6.2), and the message names the inner
// culprit.
organs::graph_doc lift_expand(organs::graph_doc doc,
                              const organs::template_loader& load);

}  // namespace syg::executor
