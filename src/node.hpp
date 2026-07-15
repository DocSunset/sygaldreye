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
  syg_hash        id;    // derived: the content's name (memoized fold); useful for looking up relations
  std::uint64_t   size;  // the span's length — a store fact (how many bytes to hash)
  syg_env_t*      env;   // resolution context HERE — what its ids resolve through
};
}

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
