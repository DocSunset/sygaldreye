#pragma once
#include <nlohmann/json.hpp>

namespace syg::harness {

// `syg walk` (HARNESS.md): the store browser on the astui navigation model
// (ch. 9). here (a single node), path (breadcrumb history), frontier
// (everything reachable from here, labeled by outward-reading edge, demand-
// driven + paginated), mark (the marked subgraph + hand links = a durable
// dataset). The ground is the synthetic node connected to all; walks begin
// there. EDR-5.
int walk_session(const nlohmann::json& in);

}  // namespace syg::harness
