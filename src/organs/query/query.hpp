// clause: machinery — the query four's evaluator (ADR-024)
#pragma once
#include <set>
#include <string>

#include "parser/parser.hpp"
#include "store.hpp"

namespace syg::organs {

// Evaluate a query graph over a store. Node types: seed (params: cids[] or
// all), traverse (via: backlinks|links), filter (key+equals, or links_to),
// join (mode: and|or), fixpoint (via: like traverse). The LAST node in the
// doc is the result. node_evals counts primitive applications.
std::set<std::string> eval_query(const graph_doc& q,
                                 const store::peer_store& s,
                                 long* node_evals = nullptr,
                                 const std::set<std::string>* delta = nullptr);

}  // namespace syg::organs
