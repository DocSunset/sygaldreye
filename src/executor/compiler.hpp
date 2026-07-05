#pragma once
#include "parser/parser.hpp"
#include "store.hpp"

namespace syg::executor {

// Compilation (CMP): an engine graph derives an execution graph from an
// app graph — a COMMITTED, deterministic derivation emitting the
// compilation map (app route -> execution route). Two memo stages: the
// structural stage keys on topology+lock alone (CMP-1.2); the full recipe
// keys on everything.
struct compilation {
  std::string execution_cid, provenance_cid, map_cid;
  nlohmann::json execution, map;
  std::vector<std::string> passes_run;  // the probe: order == engine wiring
  bool memo = false, structural_memo = false;
};

compilation compile(const nlohmann::json& app_interchange,
                    const organs::graph_doc& engine, store::peer_store& s);

}  // namespace syg::executor
