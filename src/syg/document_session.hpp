#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// `syg document` (HARNESS.md): the document layer's validation targets
// (ch. 9). A document is a graph; a source file decomposes into document form
// — an ordered set of top-level segment nodes linked in sequence — and
// regenerates byte-identically. The round-trip metric (EDR-6.2), inherited
// from the rhizome probe.
int document_session(const nlohmann::json& in);

}  // namespace syg::harness
