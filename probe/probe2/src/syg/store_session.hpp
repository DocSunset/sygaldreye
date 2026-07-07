#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// `syg store` (HARNESS.md): a scripted multi-peer store session (STO-1..9,
// LNG-10). Peers are in-process stores; rung 9's mesh replaces the
// transfer edge, never these semantics.
int store_session(const nlohmann::json& in);

}  // namespace syg::harness
