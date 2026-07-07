#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// `syg peer` (HARNESS.md): ONE peer per session — its store, its lazily
// resident engine level, its app instances. Compilation is a
// derivation-mode run of the REALIZED engine plan (CMP-9); every mutation
// is an op, every read a resolution.
int peer_session(const nlohmann::json& in);

}  // namespace syg::harness
