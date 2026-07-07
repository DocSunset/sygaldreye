#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// `syg conform` (HARNESS.md): the self-hosting closure (ch. 17). The suite as
// datasets, derived versions over succession chains (ADR-032), kind
// succession with lazy migration (CNF-4), the two profiles, and the self-gate
// (N derives N+1). CNF-1..6, COR-4 — where the book, the suite, and the
// system become one object.
int conform_session(const nlohmann::json& in);

}  // namespace syg::harness
