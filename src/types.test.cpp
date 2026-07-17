// types.test.cpp — the inscribed decree: compile-time truth.
#include "types.hpp"
#include <cassert>
using namespace syg;

// distinct rows, computable ids, exactly the roster we decreed.
static_assert(ROSTER.size() == 8);   // REFS and CONSTRUCT retired (fiat = unsayable
static_assert(CANON.size() == 15);   // or precedes-the-store; neither qualifies)
static_assert(!(ATOM.id == STRUCTURE.id));
static_assert(inscribe_symbol("atom").id == ATOM.id);   // recomputable by anyone
static_assert(ATOM.type == GROUND && ATOM.size == 4);

// relations are decreed names with constexpr ids; a name IS a string node.
static_assert(name_id("construct") == CONSTRUCT);
// the knot: str's own name is the ONE fiat spelling; every other name is a
// string typed by the str atom, whose id is constexpr-computable.
static_assert(canon_row{"str", DYNAMIC, 1}.term().name == STR_NAME.id);
static_assert(STR_TYPE.digest != 0 && !(STR_TYPE == STR_NAME.id));

// the canon mapper: category + width; aliases collapse to fixed-width truth.
static_assert(canon_name<int>() == "i32");
static_assert(canon_name<char>() == "u8");            // by decree
static_assert(canon_name<std::size_t>() == "u64");    // alias collapse
static_assert(canon_name<bool>() == "b8");
static_assert(canon_name<double>() == "f64");
static_assert(canon_name<syg_hash>() == "hash64_fnv1a");
static_assert(make_canon<float>().size == 4 && make_canon<float>().align == 4);
static_assert(canon_row{"f32", 4, 4}.term().name == name_id("f32"));

int main() {
  // the constexpr id twin and the runtime preimage are ONE function
  // (both bottom out in syg_hash::chars) — verified over real content:
  assert(syg_id(GROUND, ATOM.size, (const void*)ATOM.data) == ATOM.id);

  // data-first handle: a handle punned at its first member IS its payload.
  assert(*(char**)&ATOM == (char*)ATOM.data);

  // every roster row rehashes to its id (the IPFS property, at the floor).
  for (const syg_handle_t& h : ROSTER) assert(syg_id(h) == h.id);

  // the constexpr chained-mix STR_TYPE equals the runtime hash of the packed
  // term struct — the conformance the static_asserts above pin.
  atom_term st{.name = STR_NAME.id, .size = DYNAMIC, .align = 1};
  assert(syg_id(ATOM.id, sizeof st, (const void*)&st) == STR_TYPE);
  return 0;
}
