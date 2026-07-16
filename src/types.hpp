#pragma once
// types.hpp — the TERM LAYOUTS of decree v1: what each constructor's content
// looks like. A term holds only what cannot be derived: atom carries instance
// size/align (ground facts); structure/variant carry only (name, type) pairs
// (instance layout derives per-peer by folding children); pointers carry only
// their pointee; scope carries one link of a name chain.
//
// ONE node factory lives here, and it is the exception that defines the rule:
// inscribe_symbol mints the fiat symbols — the decree precedes every
// environment, so they alone may exist unregistered. Every OTHER node is born
// INSIDE a registry (env.hpp), which owns the payload, stamps the env, and
// guarantees every hash a term carries decodes.
#include "node.hpp"
#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace syg {

// ── the ground ──────────────────────────────────────────────────────────────
// The zero digest: a type-id saying "the decoder is out of band — below this
// a reader must already know." Every type chain terminates here. Doubles as
// "anonymous" in a composite term's name slot.
constexpr syg_hash GROUND{};

// ── fiat: the ONE unregistered node factory ─────────────────────────────────
// A fiat symbol is a decreed name: type GROUND, data the canonical chars,
// minted OUTSIDE any registry. constexpr because a string literal has static
// storage: the decree is inscribed in the binary itself.
constexpr syg_handle_t inscribe_symbol(const char* name) {
  std::uint64_t n = std::char_traits<char>::length(name);
  return { .data = const_cast<char*>(name), .type = GROUND,
           .id = syg_id(GROUND, n, name), .size = n, .env = nullptr };
}

// ── the roster: each canonical literal written EXACTLY ONCE, here ───────────
inline constexpr syg_handle_t ATOM         = inscribe_symbol("atom");          // primitive-type ctor
inline constexpr syg_handle_t STRUCTURE    = inscribe_symbol("structure");     // product: all fields
inline constexpr syg_handle_t VARIANT      = inscribe_symbol("variant");       // sum: one case, tagged
inline constexpr syg_handle_t MUTABLE_PTR  = inscribe_symbol("mutable_ptr");   // may write through it
inline constexpr syg_handle_t CONSTANT_PTR = inscribe_symbol("constant_ptr");  // may only read
inline constexpr syg_handle_t SCOPE        = inscribe_symbol("scope");         // one step of a name
inline constexpr syg_handle_t SYMBOL       = inscribe_symbol("symbol");        // a NAME (interned)
inline constexpr syg_handle_t CONTENT      = inscribe_symbol("content");       // organ: the store

// the decree's table of contents — iterate to visit every fiat symbol.
// The sharpened fiat criterion: a row is fiat iff UNSAYABLE from inside or
// PRECEDES-THE-STORE. (REFS is gone — a reference is a REF-typed row of the
// environment's one table; CONSTRUCT is gone — relations are sayable,
// post-floor, ordinary decreed symbols below.)
inline constexpr std::array ROSTER{ATOM, STRUCTURE, VARIANT, MUTABLE_PTR, CONSTANT_PTR,
                                   SCOPE, SYMBOL, CONTENT};

// a symbol's id, computable anywhere — the constexpr twin working for names.
// (The NODE must still be minted resident by whoever refers to it.)
constexpr syg_hash symbol_id(const char* s)
  { return syg_id(SYMBOL.id, std::char_traits<char>::length(s), s); }
constexpr syg_hash symbol_id(std::string_view s)
  { return syg_id(SYMBOL.id, s.size(), s.data()); }

// ── decreed RELATIONS — canonical symbols, NOT fiat ─────────────────────────
// The roster stops growing per relation, forever.
inline constexpr syg_hash CONSTRUCT = symbol_id("construct");
// later, verbatim: TICK = symbol_id("tick"); ERASE = symbol_id("erase");

// ── atom: a primitive type's term ───────────────────────────────────────────
// Three ground facts. Instance size/align live INSIDE the term — facts about
// the type; the node's own size is the term's byte length. name: a SYMBOL
// (or SCOPE) node id.
struct atom_term { syg_hash name; std::uint64_t size, align; };

// instance length "dynamic": it comes from the handle/store, never the type
// (an unsized type). Max, not 0 — a ZERO-size atom is legal and useful: a
// `struct type_tag {};` carries no bytes but is still a type.
inline constexpr std::uint64_t DYNAMIC = ~std::uint64_t{0};

// ── structure & variant: ONE term layout, two decoders ─────────────────────
// term = [ name ][ field_term… ]; arity is NOT stored — it falls out of the
// node's size: (size - 8) / 16. name = GROUND ⇒ anonymous (structural).
struct field_term { syg_hash name; syg_hash type; };  // one field / one case; name: a SYMBOL id

// ── pointers: unary, anonymous — the term is just the pointee id ────────────
// (No struct needed: the term is one syg_hash.) The pointer constructors
// encode ACCESS (may you write through it), never OWNERSHIP: a mutable_ptr
// may be owned heap or borrowed output memory a builder placed (the
// privileged-unsafe seam). Ownership is a graph-level fact — tick and the
// lifecycle operators decide what is input and output.

// ── the CANON: decreed primitives ───────────────────────────────────────────
// Ordinary ATOM residents, sayable from inside; the decree pins only the
// NAMES so every peer mints identical ids. No lookup service: whoever holds
// this table (everyone) RE-DERIVES — content addressing's whole point. A
// canon_row is the AUTHORED twin of an atom_term (spelling instead of
// name-id); term() is the one sanctioned bridge.
struct canon_row {
  std::string_view name;
  std::uint64_t size, align;
  constexpr atom_term term() const { return {symbol_id(name), size, align}; }
};

// C++ fundamental → canonical name: category + width, so aliases (size_t,
// long, char) collapse to fixed-width truth. Pointers recurse elsewhere.
template <class T> consteval std::string_view canon_name() {
  static_assert(!std::is_same_v<T, wchar_t> && !std::is_same_v<T, char16_t> &&
                !std::is_same_v<T, char32_t>, "no canonical wide char - refused");
  if      constexpr (std::is_same_v<T, bool>) return "b8";
  else if constexpr (std::is_same_v<T, char>) return "u8";   // by decree: UTF-8 code units
  else if constexpr (std::is_same_v<T, hash64_fnv1a>) return "hash64_fnv1a";
  else if constexpr (std::is_floating_point_v<T>) {
    static_assert(sizeof(T) <= 8, "no canonical float of this width");   // long double refused
    return sizeof(T) == 4 ? "f32" : "f64";
  }
  else if constexpr (std::is_integral_v<T> && std::is_signed_v<T>)
    return sizeof(T) == 1 ? "i8" : sizeof(T) == 2 ? "i16" : sizeof(T) == 4 ? "i32" : "i64";
  else if constexpr (std::is_integral_v<T>)
    return sizeof(T) == 1 ? "u8" : sizeof(T) == 2 ? "u16" : sizeof(T) == 4 ? "u32" : "u64";
  else static_assert(false, "not a canonical primitive");
}

// rows built FROM the C++ types they carry — name, size, align all supplied
// by the compiler, so the table cannot disagree with the ABI. The decree's
// numbers are pinned by the asserts below: a platform where these fail
// cannot be a conforming peer (its mints would fork every canonical id).
template <class T> consteval canon_row make_canon()
  { return {canon_name<T>(), sizeof(T), alignof(T)}; }
static_assert(sizeof(bool) == 1 && alignof(bool) == 1 &&
              sizeof(float) == 4 && alignof(float) == 4 &&
              sizeof(double) == 8 && alignof(double) == 8 &&
              alignof(std::int64_t) == 8 && alignof(hash64_fnv1a) == 8,
              "non-conforming platform: canonical ids would fork");

inline constexpr std::array CANON{
  canon_row{"nil", 0, 1},           // no C++ carrier: void has no size; constant_ptr(nil) = void*
  make_canon<bool>(),
  make_canon<std::int8_t>(),  make_canon<std::int16_t>(),
  make_canon<std::int32_t>(), make_canon<std::int64_t>(),
  make_canon<std::uint8_t>(), make_canon<std::uint16_t>(),
  make_canon<std::uint32_t>(), make_canon<std::uint64_t>(),
  make_canon<float>(), make_canon<double>(),
  canon_row{"str", DYNAMIC, 1},     // user text — unsized: no sizeof to trust
  canon_row{"ref", 8, 8},           // an intent type: a hash's bytes, "deref me" meaning
  make_canon<syg_hash>(),           // the digest struct, mapped through its own mapper
};

// ── scope: qualification as content ─────────────────────────────────────────
// One step of a qualified name — a Merkle chain: geo::vec2 =
// scope(scope(GROUND, "geo"), "vec2"). A term's name slot may point at a
// SYMBOL (unqualified), a SCOPE chain (qualified), or GROUND (anonymous) —
// the id's TYPE says which; :: is spelling, not structure. Qualification is
// IDENTITY (it folds into ids); the environment (env.hpp) is RESOLUTION
// (what's visible HERE, mutable, idless) — never conflate the two.
struct scope_term { syg_hash parent; syg_hash name; };

}  // namespace syg
