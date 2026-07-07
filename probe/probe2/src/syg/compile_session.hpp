#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// `syg compile` (HARNESS.md): the CMP session — compile as a committed
// derivation over engine-v0-as-data, the map, projection editing through
// the inverse map, forks, and the tower's laziness counter.
int compile_session(const nlohmann::json& in);

}  // namespace syg::harness
