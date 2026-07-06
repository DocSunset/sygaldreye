#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// `syg mesh` (HARNESS.md): a scripted session that boots N peers, each with a
// real loopback listener, real Ed25519 identities, and the authenticated
// encrypted transport (ADR-035). Pairing, revocation, advertisement, fetch,
// placement, and subscription happen OVER THE WIRE between in-process peers —
// peer-level conformance (ch. 17). MSH-1..8, FMT-4, ABI-4.
int mesh_session(const nlohmann::json& in);

}  // namespace syg::harness
