#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// The `syg naming` contract (HARNESS.md): build an environment from named
// projections ("$name" link placeholders commit in dependency order) and a
// ref table, then run resolve/normalize/subscribe/move-ref/events ops.
nlohmann::json naming_session(const nlohmann::json& input);

}  // namespace syg::harness
