#pragma once
#include <string>

namespace syg::naming {

struct promise {
  std::string kind;
  std::string discipline;  // "event", "value", or a clock name (ADR-020)
};

struct verdict {
  bool legal = false;
  std::string mapping;  // empty = true edge; else the boundary mapping's name
};

// The one first-order oracle (NAM-5): edit-time only, never on a tick path.
verdict connection_legal(const promise& from, const promise& to);

// Instrumentation (NAM-5.4): total lookups since process start.
long lookup_count();

}  // namespace syg::naming
