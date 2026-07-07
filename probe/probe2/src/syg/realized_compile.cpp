#include "realized_compile.hpp"

#include <map>

#include "exec_plan.hpp"
#include "phase.hpp"
#include "store.hpp"
#include "svalue_accessors.hpp"

namespace syg::harness {

realized realized_compile(store::peer_store& s, executor::exec_plan* resident,
                          const organs::graph_doc& engine_doc,
                          const nlohmann::json& app) {
  realized out;
  auto app_cid = s.put_node(app, false);
  // the receive inlet is TRANSPORT, not definition: the app rides the
  // recipe as its own input, so the injected copy leaves the engine key
  // (a resident engine would otherwise carry the last session's app)
  auto key_doc = engine_doc;
  for (const auto& [id, t] : key_doc.nodes)
    if (t == "receive") key_doc.defaults.erase(id + "/app");
  auto engine_cid = s.put_node(organs::serialize_graph(key_doc), false);
  nlohmann::json recipe{{"op", "compile"},
                        {"inputs", {{"app", {{"/", app_cid}}},
                                    {"engine", {{"/", engine_cid}}}}},
                        {"determinism", "exact"}};
  if (auto hit = s.memo_lookup(recipe)) {
    out.execution_cid = hit->output;
    out.provenance_cid = hit->provenance;
    out.execution = s.get_node(hit->output);
    out.memo = true;
    return out;
  }
  // derivation-mode run of the ENGINE PLAN (EXE-7): transient unless the
  // editor holds the level open
  executor::exec_plan* plan = resident;
  std::unique_ptr<executor::exec_plan> transient;
  if (!plan) {
    transient = std::make_unique<executor::exec_plan>(engine_doc, 48000, 128);
    transient->set_store(&s);
    plan = transient.get();
  }
  // counters keyed by the PLAN's realized instances (post-expansion) —
  // a graph-authored pass is witnessed as aud0.t0, not its authored id
  auto instances = plan->instance_ids();
  std::map<std::string, long> before;
  for (const auto& id : instances)
    before[id] = std::max(plan->recomputes(id), 0L);
  abi::reset_compile_work();
  auto structural_before = abi::structural_runs();
  plan->submit({"set_text", "receive0/app", app.dump(), "peer"});
  std::string result;
  for (int i = 0; i < 200; ++i) {
    plan->pump_block();
    if (plan->last_tick_order().size() > out.tick_order.size())
      out.tick_order = plan->last_tick_order();
    if (const auto* sv = plan->svalue_of("realize0/out"); sv && sv->value) {
      result = generated::as_text(*sv);
      if (!result.empty() && i > 20) break;
    }
  }
  if (result.empty()) throw std::runtime_error("the engine never realized");
  out.execution = nlohmann::json::parse(result);
  auto c = s.commit_derivation(recipe, out.execution);
  out.execution_cid = c.output;
  out.provenance_cid = c.provenance;
  out.structural_work = abi::structural_runs() - structural_before;
  out.outside_hook_work = abi::compile_work_outside_hooks();
  for (const auto& id : instances) {
    auto abs_n = std::max(plan->recomputes(id), 0L);
    out.pass_ticks[id] = abs_n - before[id];
    out.pass_ticks_total[id] = abs_n;
    out.passes_ticked += abs_n - before[id];
  }
  return out;
}

}  // namespace syg::harness
