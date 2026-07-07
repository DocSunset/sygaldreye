// clause: machinery — realize's CODEGEN backend (ADR-014): emit a fused,
// portable, typed C++ movement from the choose-annotated doc. Same passes,
// same map, same provenance shape as interpret — one compilation model.
#pragma once
#include <string>

#include <nlohmann/json.hpp>

#include "parser/parser.hpp"

namespace syg::executor {

struct emitted {
  std::string source;  // the C++ artifact
  int tier = 1;        // 1 freestanding, 2 platform-lib, 3 host-bound
  std::string tier_culprit;
};

// doc: the expanded app carrying __regions/__mappings (choose's output);
// ledger: vocabulary/codegen.json's "natives" table (templates ARE data).
// Ordering comes from scc_order — the interpreter's own schedule — so the
// fused loop is byte-identical to the interpreted render (FRZ-1.1).
emitted emit_frozen(const organs::graph_doc& doc,
                    const nlohmann::json& ledger, int block);

}  // namespace syg::executor
