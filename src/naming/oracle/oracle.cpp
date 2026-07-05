#include "oracle.hpp"

namespace syg::naming {
namespace {

long lookups = 0;

bool clocked(const std::string& d) { return d != "event" && d != "value"; }

}  // namespace

long lookup_count() { return lookups; }

verdict connection_legal(const promise& from, const promise& to) {
  ++lookups;
  if (from.kind != to.kind) return {};  // no conversions at this rung
  if (from.discipline == to.discipline) return {true, ""};
  if (from.discipline == "event" || to.discipline == "event") return {};
  if (clocked(to.discipline)) return {true, "latch"};
  if (to.discipline == "value") return {true, "snapshot"};
  return {};
}

}  // namespace syg::naming
