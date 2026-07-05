// clause: machinery — eval_query: realize the query doc and read the sink
#include <nlohmann/json.hpp>

#include "exec_plan.hpp"
#include "query/query.hpp"
#include "svalue_accessors.hpp"

namespace syg::organs {

std::set<std::string> eval_query(const graph_doc& q, const store::peer_store& s,
                                 long* node_evals,
                                 const std::set<std::string>* delta) {
  graph_doc doc = q;
  if (delta) {
    // the standing form: the new commits ARE the seed (dirty-cone pass)
    nlohmann::json cids = nlohmann::json::array();
    for (const auto& c : *delta) cids.push_back(c);
    for (const auto& [id, type] : doc.nodes)
      if (type == "seed") {
        doc.defaults[id + "/cids"] = cids;
        doc.defaults[id + "/all"] = false;
      }
  }
  executor::exec_plan p(doc, 48000, 128);
  p.set_store(&s);
  // convergence: value instances settle over dirty-cone frame ticks; a
  // query doc is at most a handful deep
  for (int i = 0; i < 80; ++i) p.pump_block();
  long evals = 0;
  for (const auto& [id, type] : doc.nodes) {
    auto n = p.recomputes(id);
    if (n > 0) evals += n;
  }
  if (node_evals) *node_evals = evals;
  // the sink: the node nothing consumes
  std::string sink;
  for (const auto& [id, type] : doc.nodes) {
    bool consumed = false;
    for (const auto& [f, t] : doc.edges)
      if (f.substr(0, f.find('/')) == id) consumed = true;
    if (!consumed) sink = id;
  }
  if (sink.empty() && !doc.nodes.empty()) sink = doc.nodes.back().first;
  const auto* sv = p.svalue_of(sink + "/out");
  std::set<std::string> out;
  if (sv && sv->value)
    for (const auto& c : generated::as_cidset(*sv)) out.insert(c);
  return out;
}

}  // namespace syg::organs
