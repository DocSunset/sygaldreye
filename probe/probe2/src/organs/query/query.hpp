// clause: machinery — the query organ's shared machinery + the realize
// wrapper (the bespoke evaluator DISSOLVED here per LNG-11.4: queries now
// run as realized instances; this wrapper builds the plan and reads the
// sink cell)
#pragma once
#include <set>
#include <string>

#include "parser/parser.hpp"
#include "store.hpp"

namespace syg::organs {

// one traversal hop (store machinery the query natives call)
std::set<std::string> query_step(const store::peer_store& s,
                                 const std::set<std::string>& from,
                                 const std::string& via);
// provenance records pull their outputs into the downstream cone
std::set<std::string> query_outputs_of(const store::peer_store& s,
                                       const std::set<std::string>& records);

// REALIZE the query doc (seed/traverse/filter/join/fixpoint instances over
// the structured lane), inject the store, pump to convergence, read the
// sink. node_evals counts instance recomputes; delta seeds the standing
// form's incremental pass.
std::set<std::string> eval_query(const graph_doc& q,
                                 const store::peer_store& s,
                                 long* node_evals = nullptr,
                                 const std::set<std::string>* delta = nullptr);

}  // namespace syg::organs
