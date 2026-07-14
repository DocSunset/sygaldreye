#pragma once
// node.hpp — THE decree: the one struct a reader must already know in order to
// read anything else. Everything in the universe is one of these — a value, a
// type (a term whose `type` is its constructor), a behavior, the registry
// itself. The portable half is {id, type, size} + the payload bytes; `data` is
// this peer's pointer to them — local, never on the wire. Behavior (tick) is
// NOT here: it attaches from outside (a ref per type), resolved at build time.
#include "hash.hpp"
#include <cstdint>

extern "C" {
struct syg_node_t {
  syg_hash      id;    // = syg_id(…) — content address; this node's name in the universe
  syg_hash      type;  // the node that decodes this one; chains ground at the genesis roster
  std::uint64_t size;  // byte length of data — hashable, shippable, without decoding
  void*         data;  // the payload, meaningful only through `type`
};
}

// syg_id — decree v1's preimage, pinned byte for byte:
//   id = fnv1a( type.digest ∥ size ∥ data[0..size) ),  the u64s as 8 bytes LSB-first.
// syg_hash::mix absorbs a u64 LSB-first, so this chain IS that byte stream.
inline syg_hash syg_id(syg_hash type, std::uint64_t size, const void* data) {
  return syg_hash::bytes(data, size, syg_hash::mix(syg_hash::mix({syg_hash::basis}, type), size));
}
inline syg_hash syg_id(const syg_node_t& n) { return syg_id(n.type, n.size, n.data); }
