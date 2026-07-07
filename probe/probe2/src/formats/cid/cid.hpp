#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace syg::formats {

using byte_vec = std::vector<std::uint8_t>;

byte_vec blake3_256(const std::uint8_t* data, std::size_t len);

// CIDv1 bytes: 0x01, varint multicodec, multihash blake3-256 (0x1e, 0x20, hash).
byte_vec cid_of(std::uint64_t multicodec, const byte_vec& data);
std::string cid_to_text(const byte_vec& cid);  // multibase 'b' + base32 lower, no pad
byte_vec cid_from_text(const std::string& text);

// Multibase base32 (the 'b' encoding) over arbitrary bytes — the one text
// codec for content addresses AND peer-keys (ADR-035: a peer-key is an
// Ed25519 public key encoded exactly as a CID is).
std::string mb32_encode(const byte_vec& data);
byte_vec mb32_decode(const std::string& text);
// Re-hash data and compare against the cid (any multicodec the cid claims).
bool cid_verify(const byte_vec& cid, const byte_vec& data);

}  // namespace syg::formats
