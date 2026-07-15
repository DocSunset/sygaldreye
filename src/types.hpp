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
inline constexpr syg_handle_t REFS         = inscribe_symbol("refs");          // organ: the ref table
inline constexpr syg_handle_t CONSTRUCT    = inscribe_symbol("construct");     // relation: type → fold

// the decree's table of contents — iterate to visit every fiat symbol.
inline constexpr std::array ROSTER{ATOM, STRUCTURE, VARIANT, MUTABLE_PTR, CONSTANT_PTR,
                                   SCOPE, SYMBOL, CONTENT, REFS, CONSTRUCT};

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

// ── scope: qualification as content ─────────────────────────────────────────
// One step of a qualified name — a Merkle chain: geo::vec2 =
// scope(scope(GROUND, "geo"), "vec2"). A term's name slot may point at a
// SYMBOL (unqualified), a SCOPE chain (qualified), or GROUND (anonymous) —
// the id's TYPE says which; :: is spelling, not structure. Qualification is
// IDENTITY (it folds into ids); the environment (env.hpp) is RESOLUTION
// (what's visible HERE, mutable, idless) — never conflate the two.
struct scope_term { syg_hash parent; syg_hash name; };

}  // namespace syg
