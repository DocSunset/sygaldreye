#pragma once
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "parser/parser.hpp"

namespace syg::store {
class peer_store;
}
namespace syg::executor {
class exec_plan;
}

namespace syg::harness {

// Compilation IS a derivation-mode run of the realized engine plan
// (CMP-9, EXE-7): every session that compiles goes through here — the
// ONE compiler, transient unless the editor holds `resident` open.
struct realized {
  std::string execution_cid, provenance_cid;
  nlohmann::json execution;  // the body
  bool memo = false;
  long structural_work = 0;  // abi::structural_runs() delta (CMP-1.2)
  nlohmann::json pass_ticks, pass_ticks_total;
  long passes_ticked = 0;
  std::vector<std::string> tick_order;
  long outside_hook_work = 0;
};

realized realized_compile(store::peer_store& s, executor::exec_plan* resident,
                          const organs::graph_doc& engine_doc,
                          const nlohmann::json& app);

}  // namespace syg::harness
