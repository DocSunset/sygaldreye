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

// relations are decreed symbols with constexpr ids.
static_assert(symbol_id("construct") == CONSTRUCT);

// the canon mapper: category + width; aliases collapse to fixed-width truth.
static_assert(canon_name<int>() == "i32");
static_assert(canon_name<char>() == "u8");            // by decree
static_assert(canon_name<std::size_t>() == "u64");    // alias collapse
static_assert(canon_name<bool>() == "b8");
static_assert(canon_name<double>() == "f64");
static_assert(canon_name<syg_hash>() == "hash64_fnv1a");
static_assert(make_canon<float>().size == 4 && make_canon<float>().align == 4);
static_assert(canon_row{"f32", 4, 4}.term().name == symbol_id("f32"));

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
