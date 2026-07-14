#pragma once
// types.hpp — the type constructors of decree v1: fiat (the ground), atom,
// structure, variant, and the two pointers. Every constructor mints a
// syg_node_t whose data is its TERM — and a term holds only what cannot be
// derived: atom carries size/align (ground facts); structure/variant carry
// only (name, type) pairs (instance layout derives per-peer by folding
// children); pointers carry only their pointee. Every mint ends at the
// registry's insert_or_get (not yet built) — payload ownership transfers
// there; until then the allocations here are cheques written on it.
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

// A fiat node: type GROUND, data its canonical name. One per decree-roster
// row; its id falls out of the same preimage as every other id — ONE identity
// scheme, recomputable by anyone holding the decree (names + algorithm).
inline syg_node_t fiat(const char* name) {
  return { syg_id(GROUND, std::strlen(name), name), GROUND, std::strlen(name), (void*)name };
}

// ── the roster (so far) ─────────────────────────────────────────────────────
inline const syg_node_t ATOM         = fiat("atom");          // primitive-type constructor
inline const syg_node_t STRING       = fiat("string");        // utf-8 bytes; length in the header
inline const syg_node_t STRUCTURE    = fiat("structure");     // product: all of the fields
inline const syg_node_t VARIANT      = fiat("variant");       // sum: one of the cases (tag + widest)
inline const syg_node_t MUTABLE_PTR  = fiat("mutable_ptr");   // may write through it
inline const syg_node_t CONSTANT_PTR = fiat("constant_ptr");  // may only read through it

// ── strings ─────────────────────────────────────────────────────────────────
inline syg_node_t string_node(const char* s) {
  std::uint64_t n = std::strlen(s);
  return { syg_id(STRING.id, n, s), STRING.id, n, (void*)s };
}

// ── atom: mint a primitive type ─────────────────────────────────────────────
// Authored spelling of ATOM's instance layout (reflection makes the layout
// itself a node later; until then it is one decree row). The returned node IS
// the term node: a three-field record through ATOM's eyes, a type through the
// universe's. Instance size/align live INSIDE the term — facts about the
// type; the header's size is the term's own byte length.
struct atom_term { syg_hash name; std::uint64_t size, align; };

inline syg_node_t atom(syg_hash name /* id of a STRING node */,
                       std::uint64_t size, std::uint64_t align) {
  auto* t = new atom_term{name, size, align};
  return { syg_id(ATOM.id, sizeof *t, t), ATOM.id, sizeof *t, t };
}
// atom(const char* name, …) — the authored convenience — waits on the
// registry: its whole job is insert_or_get(string_node(name)); hashing alone
// would mint types whose name-ids point at nothing.

// ── structure & variant: ONE term layout, two decoders ─────────────────────
struct field_term { syg_hash name; syg_hash type; };   // one field / one case

// term = [ name ][ field_term… ]; arity is NOT stored — it falls out of the
// header: (size - 8) / 16. name = GROUND ⇒ anonymous (purely structural).
inline syg_node_t composite(const syg_node_t& ctor, syg_hash name,
                            std::span<const field_term> fields) {
  std::uint64_t sz = sizeof name + fields.size_bytes();
  auto* t = (unsigned char*)std::malloc(sz);
  std::memcpy(t, &name, sizeof name);
  std::memcpy(t + sizeof name, fields.data(), fields.size_bytes());
  return { syg_id(ctor.id, sz, t), ctor.id, sz, t };
}
inline syg_node_t structure(syg_hash name, std::span<const field_term> fields)
  { return composite(STRUCTURE, name, fields); }
inline syg_node_t variant(syg_hash name, std::span<const field_term> cases)
  { return composite(VARIANT, name, cases); }

// ── pointers: unary, anonymous, term = the pointee id ──────────────────────
// The pointer constructors encode ACCESS (may you write through it), never
// OWNERSHIP: a mutable_ptr may be owned heap or borrowed output memory a
// builder placed (the privileged-unsafe seam). Ownership is a graph-level
// fact — tick and the lifecycle operators decide what is input and output.
inline syg_node_t pointer(const syg_node_t& ctor, syg_hash pointee) {
  auto* t = new syg_hash{pointee};
  return { syg_id(ctor.id, sizeof *t, t), ctor.id, sizeof *t, t };
}
inline syg_node_t mutable_ptr(syg_hash pointee)  { return pointer(MUTABLE_PTR,  pointee); }
inline syg_node_t constant_ptr(syg_hash pointee) { return pointer(CONSTANT_PTR, pointee); }

}  // namespace syg
