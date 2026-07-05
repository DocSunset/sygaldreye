#include "oracle.hpp"

namespace syg::naming {
namespace {

long lookups = 0;

bool clocked(const std::string& d) { return d != "event" && d != "value"; }

}  // namespace

long lookup_count() { return lookups; }

namespace {

// the structured kinds (ADR-034): event/value lanes only, never stream —
// mirrors the catalog's non-float set (vocabulary/kinds.json)
bool structured(const std::string& k) {
  return k == "graph" || k == "ops" || k == "text" || k == "mesh" ||
         k == "texture" || k == "surface" || k == "draw_call";
}

}  // namespace

verdict connection_legal(const promise& from, const promise& to) {
  ++lookups;
  if ((structured(from.kind) || structured(to.kind)) &&
      (clocked(from.discipline) || clocked(to.discipline)))
    return {};  // the float path pays nothing: no structure on stream
  // span ports are rank-polymorphic: they accept (or gather) their cell
  // kind — the catalog's cell_rank refines this when kinds carry payloads
  if (from.kind != to.kind && from.kind != "span" && to.kind != "span")
    return {};  // no conversions at this rung
  if (from.discipline == to.discipline) return {true, ""};
  if (from.discipline == "event" || to.discipline == "event") return {};
  if (clocked(to.discipline)) return {true, "latch"};
  if (to.discipline == "value") return {true, "snapshot"};
  return {};
}

}  // namespace syg::naming
