#pragma once
// hash.hpp — the identity algorithm of decree v1, as one struct: the digest
// type (the member) + the algorithm (the statics: constants and entry points).
// FNV-1a, 64-bit. A DEFINED algorithm (never std::hash, which isn't stable
// across implementations) — every peer must compute identical digests or ids
// don't agree. Non-cryptographic: runtime identity among honest parties. A
// successor (hash256_*) arrives as a sibling struct and the syg_hash alias
// moves; this type and its functions remain FOREVER — verifying v1 content
// requires the v1 algorithm.
//
// DECREE DUTY (idea notes, not yet built): this struct must SERVE AS DECREE.
//  - Its reflection should generate the primitive type (hash64_fnv1a, size 8)
//    and the nodes for decoding, constructing, generally getting along — at
//    least four nodes (the prim + ticks for str/mix/bytes).
//  - Install ref(s) to the defaults — the runtime equivalent of the
//    `using syg_hash` alias below — at by-decree hashes in the ref table, so
//    a peer resolves "the current id algorithm" without recompiling.
#include <cstddef>
#include <cstdint>

struct hash64_fnv1a {
  std::uint64_t digest;
  constexpr bool operator==(const hash64_fnv1a&) const = default;

  static constexpr std::uint64_t basis = 0xcbf29ce484222325ull;  // the decree
  static constexpr std::uint64_t prime = 0x100000001b3ull;       // constants, named

  // a name over its chars, terminator excluded (content, not encoding)
  static constexpr hash64_fnv1a str(const char* s, hash64_fnv1a h = {basis}) {
    for (; *s; ++s) { h.digest ^= static_cast<unsigned char>(*s); h.digest *= prime; }
    return h;
  }
  // absorb the eight bytes of b into a, LSB-first (endian-defined ⇒ same on every peer)
  static constexpr hash64_fnv1a mix(hash64_fnv1a a, hash64_fnv1a b) {
    for (int i = 0; i < 8; ++i) { a.digest ^= (b.digest >> (i * 8)) & 0xff; a.digest *= prime; }
    return a;
  }
  // runtime blob hash — the void* pun bars it from constant expressions
  static hash64_fnv1a bytes(const void* p, std::size_t n, hash64_fnv1a h = {basis}) {
    const unsigned char* b = static_cast<const unsigned char*>(p);
    for (std::size_t i = 0; i < n; ++i) { h.digest ^= b[i]; h.digest *= prime; }
    return h;
  }
};
using syg_hash = hash64_fnv1a;
