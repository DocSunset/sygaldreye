#pragma once
#include <array>
#include <cstdint>
#include <string_view>

// The ch. 14 pins, frozen 2026-07-05 (FMT-5). Succession only, never edits.
namespace syg::formats::pins {

inline constexpr std::uint64_t multicodec_dag_cbor = 0x71;
inline constexpr std::uint64_t multicodec_raw = 0x55;
inline constexpr std::uint64_t multihash_blake3_256 = 0x1e;
inline constexpr std::size_t hash_bytes = 32;
inline constexpr char multibase_prefix = 'b';  // base32, lowercase, no padding
inline constexpr std::size_t chunk_bytes = 256 * 1024;
inline constexpr std::string_view escape_set = "%/:# \t\n";
inline constexpr std::array<std::string_view, 3> tape_records = {"NODE", "LINK",
                                                                 "SET"};
inline constexpr std::array<std::string_view, 6> edit_ops = {
    "add_node", "remove_node", "add_edge", "remove_edge", "set_param",
    "replace_graph"};
// Wire message kinds: varint numbering in ch. 14 table order.
inline constexpr std::array<std::string_view, 8> wire_kinds = {
    "PAIR", "HELLO", "SUBSCRIBE", "OPS", "FETCH", "QUERY", "PLACE", "FAULT"};

}  // namespace syg::formats::pins
