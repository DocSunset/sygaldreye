// types.test.cpp — the inscribed decree: compile-time truth.
#include "types.hpp"
#include <cassert>
using namespace syg;

// distinct rows, computable ids, exactly the roster we decreed.
static_assert(ROSTER.size() == 9);   // REFS retired: no refs organ exists to name
static_assert(!(ATOM.id == STRUCTURE.id));
static_assert(inscribe_symbol("atom").id == ATOM.id);   // recomputable by anyone
static_assert(ATOM.type == GROUND && ATOM.size == 4);

int main() {
  // the constexpr id twin and the runtime preimage are ONE function
  // (both bottom out in syg_hash::chars) — verified over real content:
  assert(syg_id(GROUND, ATOM.size, (const void*)ATOM.data) == ATOM.id);

  // data-first handle: a handle punned at its first member IS its payload.
  assert(*(char**)&ATOM == (char*)ATOM.data);

  // every roster row rehashes to its id (the IPFS property, at the floor).
  for (const syg_handle_t& h : ROSTER) assert(syg_id(h) == h.id);
  return 0;
}
