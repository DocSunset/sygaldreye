// clause: scaffolding (dissolves: FRZ-1.1) — SHELVED rung-8 WIP: the
// codegen backend must fuse the REAL engine's realize output once CMP-9
// lands; this standalone walk is parked, not an endpoint
#pragma once
#include "compiler.hpp"

namespace syg::executor {

// The codegen backend of realization (ADR-014, FRZ): emit a fused,
// portable, typed C++ movement from the compiled execution graph — same
// semantics as the interpreter (a pure optimizer, ADR-013), provenance
// carried in the artifact for unfreezing. Tier is computed from the
// native closure (kernel-only = tier 1, freestanding).
struct frozen {
  std::string source;        // the C++ artifact
  std::string artifact_cid;  // committed (raw)
  std::string provenance_cid;
  int tier = 1;
  std::string tier_culprit;  // the native that forced the tier
  bool memo = false;
};

frozen freeze(const nlohmann::json& app_interchange, store::peer_store& s);

// unfreeze: recover the source graph cid from the artifact's provenance
std::string unfreeze(const std::string& artifact_source);

}  // namespace syg::executor
