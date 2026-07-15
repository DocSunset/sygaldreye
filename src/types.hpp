#pragma once
// types.hpp — the type constructors of decree v1: fiat (the ground), atom,
// structure, variant, the two pointers, and scope. Every constructor mints a
// node whose content is its TERM — and a term holds only what cannot be
// derived: atom carries instance size/align (ground facts); structure/variant
// carry only (name, type) pairs (instance layout derives per-peer by folding
// children); pointers carry only their pointee. Every mint ends at the
// environment's insert_or_get (env.hpp) — payload ownership transfers
// there; until then the allocations here are cheques written on it, and
// a freshly minted handle's env is nullptr until it is registered.
#include "node.hpp"
#include <cstdlib>
#include <cstring>
#include <span>

namespace syg {

// ── the ground ──────────────────────────────────────────────────────────────
// The zero digest: a type-id saying "the decoder is out of band — below this
// a reader must already know." Every type chain terminates here. Doubles as
// "anonymous" in a composite term's name slot.
constexpr syg_hash GROUND{};

// A fiat node: type GROUND, data its canonical name — a fiat node IS a
// decreed name. One per decree-roster row; its id falls out of the same
// preimage as every other id — ONE identity scheme, recomputable by anyone
// holding the decree (names + algorithm).
inline syg_handle_t fiat(const char* name) {
  return { .data = (void*)name, .type = GROUND,
           .id = syg_id(GROUND, std::strlen(name), name),
           .size = std::strlen(name), .env = nullptr };
}

// ── the roster (so far) ─────────────────────────────────────────────────────
inline const syg_handle_t ATOM         = fiat("atom");          // primitive-type constructor
inline const syg_handle_t STRUCTURE    = fiat("structure");     // product: all of the fields
inline const syg_handle_t VARIANT      = fiat("variant");       // sum: one of the cases (tag + widest)
inline const syg_handle_t MUTABLE_PTR  = fiat("mutable_ptr");   // may write through it
inline const syg_handle_t CONSTANT_PTR = fiat("constant_ptr");  // may only read through it
inline const syg_handle_t SCOPE        = fiat("scope");         // one step of a qualified name
inline const syg_handle_t SYMBOL       = fiat("symbol");        // a NAME: an interned identifier

// ── symbols: names as nodes ─────────────────────────────────────────────────
// The LISP symbol/string split: a SYMBOL is a name (compared by id; lives in
// term name slots), a STRING is user text (an ordinary atom, below). SYMBOL
// itself must stay a fiat row — a minted SYMBOL would need a SYMBOL-typed
// name: the hash fixed point. The demotion criterion, holding.
inline syg_handle_t symbol_node(const char* s) {
  std::uint64_t n = std::strlen(s);
  return { .data = (void*)s, .type = SYMBOL.id,
           .id = syg_id(SYMBOL.id, n, s), .size = n, .env = nullptr };
}

// ── atom: mint a primitive type ─────────────────────────────────────────────
// Authored spelling of ATOM's instance layout (reflection makes the layout
// itself a node later; until then it is one decree row). The returned handle
// grips the term node: a three-field record through ATOM's eyes, a type
// through the universe's. Instance size/align live INSIDE the term — facts
// about the type; the handle's size is the term's own byte length.
struct atom_term { syg_hash name; std::uint64_t size, align; };

// instance length "dynamic": it comes from the handle/store, never the type
// (an unsized type). Max, not 0 — a ZERO-size atom is legal and useful: a
// `struct type_tag {};` carries no bytes but is still a type.
inline constexpr std::uint64_t DYNAMIC = ~std::uint64_t{0};

inline syg_handle_t atom(syg_hash name /* id of a SYMBOL (or SCOPE) node */,
                         std::uint64_t size, std::uint64_t align) {
  auto* t = (atom_term*)std::malloc(sizeof(atom_term));   // malloc, uniformly: a term
  *t = {.name = name, .size = size, .align = align};      // is a temporary the store
  return { .data = t, .type = ATOM.id,                    // copies then frees
           .id = syg_id(ATOM.id, sizeof *t, t), .size = sizeof *t, .env = nullptr };
}
// atom(const char* name, …) — the authored convenience — lives in env.hpp:
// its whole job is insert_or_get(symbol_node(name)); hashing alone would
// mint types whose name-ids point at nothing.

// ── strings: user TEXT, an ordinary minted atom ─────────────────────────────
// Unsized, byte granularity. No fiat detour, no bootstrap asymmetry: its NAME
// is a SYMBOL node, and SYMBOL's id computes from constants — the fixed
// point dissolved with the symbol/string split.
inline const syg_handle_t STRING = atom(symbol_node("string").id, DYNAMIC, 1);

inline syg_handle_t string_node(const char* s) {
  std::uint64_t n = std::strlen(s); // this here assumes that s is null terminated
  return { .data = (void*)s, .type = STRING.id,
           .id = syg_id(STRING.id, n, s), .size = n, .env = nullptr };
  // so... this assumes that s must have static storage duration I guess? But that's a strong assumption!
}

// ── structure & variant: ONE term layout, two decoders ─────────────────────
struct field_term { syg_hash name; syg_hash type; };  // one field / one case; name: a SYMBOL id

// term = [ name ][ field_term… ]; arity is NOT stored — it falls out of the
// handle: (size - 8) / 16. name = GROUND ⇒ anonymous (purely structural).
inline syg_handle_t composite(const syg_handle_t& ctor, syg_hash name,
                              std::span<const field_term> fields) {
  std::uint64_t sz = sizeof name + fields.size_bytes();
  auto* t = (unsigned char*)std::malloc(sz);
  std::memcpy(t, &name, sizeof name);
  std::memcpy(t + sizeof name, fields.data(), fields.size_bytes());
  return { .data = t, .type = ctor.id,
           .id = syg_id(ctor.id, sz, t), .size = sz, .env = nullptr };
}
inline syg_handle_t structure(syg_hash name, std::span<const field_term> fields)
  { return composite(STRUCTURE, name, fields); }
inline syg_handle_t variant(syg_hash name, std::span<const field_term> cases)
  { return composite(VARIANT, name, cases); }

// ── pointers: unary, anonymous, term = the pointee id ──────────────────────
// The pointer constructors encode ACCESS (may you write through it), never
// OWNERSHIP: a mutable_ptr may be owned heap or borrowed output memory a
// builder placed (the privileged-unsafe seam). Ownership is a graph-level
// fact — tick and the lifecycle operators decide what is input and output.
inline syg_handle_t pointer(const syg_handle_t& ctor, syg_hash pointee) {
  auto* t = (syg_hash*)std::malloc(sizeof(syg_hash));
  *t = pointee;
  return { .data = t, .type = ctor.id,
           .id = syg_id(ctor.id, sizeof *t, t), .size = sizeof *t, .env = nullptr };
}
inline syg_handle_t mutable_ptr(syg_hash pointee)  { return pointer(MUTABLE_PTR,  pointee); }
inline syg_handle_t constant_ptr(syg_hash pointee) { return pointer(CONSTANT_PTR, pointee); }

// ── scope: qualification as content ─────────────────────────────────────────
// One step of a qualified name — a Merkle chain of names: geo::vec2 =
// scope(scope(GROUND, "geo"), "vec2"). A term's name slot may point at a
// SYMBOL (unqualified), a SCOPE chain (qualified), or GROUND (anonymous) —
// the id's TYPE says which; :: is spelling, not structure. Qualification is
// IDENTITY (it folds into ids); the environment (env.hpp) is RESOLUTION
// (what's visible HERE, mutable, idless) — never conflate the two.
struct scope_term { syg_hash parent; syg_hash name; };

inline syg_handle_t scope(syg_hash parent, syg_hash name /* id of a SYMBOL node */) {
  auto* t = (scope_term*)std::malloc(sizeof(scope_term));
  *t = {.parent = parent, .name = name};
  return { .data = t, .type = SCOPE.id,
           .id = syg_id(SCOPE.id, sizeof *t, t), .size = sizeof *t, .env = nullptr };
}

}  // namespace syg
