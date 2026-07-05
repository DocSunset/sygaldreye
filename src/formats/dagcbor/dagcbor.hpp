#pragma once
#include <cstdint>
#include <vector>
#include <nlohmann/json.hpp>

namespace syg::formats {

// Canonical dag-cbor (ch. 14, ADR-017): sorted map keys (length, then
// bytewise), definite lengths, minimal-width ints, canonical 64-bit floats,
// no NaN/Inf, string keys only, links as tag 42.
// Input is the JSON projection (DAG-JSON conventions, see HARNESS.md):
//   {"/": {"bytes": "<base64, no padding>"}}  -> byte string
//   {"/": "<cid text>"}                        -> link (tag 42)
// Throws std::runtime_error on values the canonical form forbids.
std::vector<std::uint8_t> encode_projection(const nlohmann::json& v);

// Decode the projection's bytes escape {"/": {"bytes": "<base64>"}}.
std::vector<std::uint8_t> bytes_of_projection(const nlohmann::json& v);

}  // namespace syg::formats
