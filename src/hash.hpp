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
#include <string_view>  // char_traits — the constexpr strlen

struct hash64_fnv1a {
  std::uint64_t digest;
  constexpr bool operator==(const hash64_fnv1a&) const = default;

  static constexpr std::uint64_t basis = 0xcbf29ce484222325ull;  // the decree
  static constexpr std::uint64_t prime = 0x100000001b3ull;       // constants, named
  static constexpr hash64_fnv1a  seed() { return {basis}; }      // where every fold starts

  // THE algorithm, once: absorb one byte.
  static constexpr hash64_fnv1a step(hash64_fnv1a h, unsigned char b) {
    h.digest = (h.digest ^ b) * prime;
    return h;
  }

  // THE fold, once — typed pointer + explicit length, constexpr-clean. Every
  // other entry point is an adapter over this one, so all paths agree BY
  // CONSTRUCTION, never by coincidence.
  static constexpr hash64_fnv1a chars(hash64_fnv1a h, const char* s, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) h = step(h, static_cast<unsigned char>(s[i]));
    return h;
  }
  // adapters — no loops of their own, nothing to drift:
  // a name over its chars, terminator excluded (content, not encoding):
  static constexpr hash64_fnv1a str(hash64_fnv1a h, const char* s)
    { return chars(h, s, std::char_traits<char>::length(s)); }
  // a runtime blob — the void* cast is the only thing barring constexpr:
  static hash64_fnv1a bytes(hash64_fnv1a h, const void* p, std::size_t n)
    { return chars(h, static_cast<const char*>(p), n); }
  // absorb the eight bytes of a u64, LSB-first (endian-defined ⇒ same on every peer):
  static constexpr hash64_fnv1a mix(hash64_fnv1a a, std::uint64_t b) {
    for (int i = 0; i < 8; ++i) a = step(a, (b >> (i * 8)) & 0xff);
    return a;
  }
  // absorb another digest — same bytes; a digest is just its eight bytes here:
  static constexpr hash64_fnv1a mix(hash64_fnv1a a, hash64_fnv1a b) { return mix(a, b.digest); }

  // member sugar: *this rides as the leading accumulator, so folds chain —
  //   syg_hash::seed().mix(type).bytes(data, size)
  constexpr hash64_fnv1a chars(const char* s, std::size_t n) const { return chars(*this, s, n); }
  constexpr hash64_fnv1a str(const char* s) const { return str(*this, s); }
  constexpr hash64_fnv1a mix(std::uint64_t b) const { return mix(*this, b); }
  constexpr hash64_fnv1a mix(hash64_fnv1a b) const { return mix(*this, b); }
  hash64_fnv1a bytes(const void* p, std::size_t n) const { return bytes(*this, p, n); }
};
using syg_hash = hash64_fnv1a;
