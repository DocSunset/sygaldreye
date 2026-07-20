#pragma once
// node.hpp — a NODE is (type, bytes): content, a mathematical object, not a
// struct. What ships is bytes, framed by transport; the id is a DERIVED name
// (recompute = verify); the size is a fact about the span; data and env are
// where the bytes sit and resolve HERE. So the one struct is the HANDLE —
// this peer's grip on a node. The eternal surface is only the preimage below
// + the algorithm (hash.hpp) + the fiat roster (types.hpp).
#include "hash.hpp"
#include <cstdint>

extern "C" {
typedef struct syg_env_t syg_env_t;  // the environment: a lexical frame of grips (env.hpp)

struct syg_handle_t {
  // data goes first so we can safely cast a syg_handle_t to a T* if we know its T.
  void*           data;  // ┐ the node — a (type, bytes) content;
  syg_hash        type;  // ┘ everything else is our grip on it
  syg_hash        id;    // this content's OWN name (see THE .id LAW below), or {}
  std::uint64_t   size;  // the span's length — a store fact (how many bytes to hash)
  syg_env_t*      env;   // resolution context HERE — what its ids resolve through
};
}

// ── THE .id LAW ─────────────────────────────────────────────────────────────
// A handle's .id is the content-address of its OWN data — its name as a store
// resident, syg_id(type,size,data) — or {} when the handle is not a resident
// (a grip, a table row). NOTHING else ever goes in .id. A reference to ANOTHER
// node is DATA, interpreted by the type — never squeezed into .id to dodge an
// allocation (that was the old ref hack; it is forbidden). This is what makes
// an id a content address: in the store ⇒ resolves; {} ⇒ resolves to nothing.
//
// SDO note (small-data optimization): a payload that fits the data word (a
// ref's 8-byte target, on a platform where sizeof(void*) ≥ sizeof(syg_hash))
// may ride INLINE in `data` rather than behind a heap pointer. That is
// legitimate — the TYPE has authority over how its bytes are laid out — and is
// NOT an .id hack. Generalizing SDO to every small atom is DEFERRED and
// profile-driven: if ever done it must hide behind ONE accessor, else "is it
// small?" branches spread everywhere, reaching even into syg_id.

// syg_id — decree v1's preimage, pinned byte for byte:
//   id = fnv1a( type.digest ∥ data[0..size) ),  the digest as 8 bytes LSB-first.
// A fixed-width type then ONE terminal variable field: no parse is ambiguous,
// so length is NOT absorbed — size here only says how many bytes to hash (a
// store fact, like the span itself). Keep the eternal sentence short.
// ONE preimage, two entries differing only in pointer type (both bottom out
// in syg_hash::chars — agreement by construction, not coincidence):
constexpr syg_hash syg_id(syg_hash type, std::uint64_t size, const char* data) {
  return syg_hash::seed().mix(type).chars(data, size);
}
inline syg_hash syg_id(syg_hash type, std::uint64_t size, const void* data) {
  return syg_hash::seed().mix(type).bytes(data, size);
}
inline syg_hash syg_id(const syg_handle_t& h) { return syg_id(h.type, h.size, h.data); }
