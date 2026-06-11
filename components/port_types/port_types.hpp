// Copyright 2026 Travis West
#pragma once
#include <optional>
#include <string_view>

// The type lattice (payload × rate) and connection legality — the ONE
// implementation behind both parse-time edge validation and the editor's
// wire rules. See planning/edge_executor.design.md for the ontology.
namespace port_types {

enum class Rate { Event, Frame, Block };

// v1: rate derives from payload kind alone. When schemas grow a per-port
// rate field (audio-region work), this becomes a fallback.
constexpr Rate rate_of(std::string_view kind) {
    if (kind == "audio") return Rate::Block;
    if (kind == "bang")  return Rate::Event;
    return Rate::Frame;
}

constexpr bool is_wildcard(std::string_view kind) {
    return kind.empty() || kind == "any" || kind == "unknown";
}

// May these two ports be connected at all (true edge OR via a mapping)?
constexpr bool connection_legal(std::string_view from, std::string_view to) {
    if (is_wildcard(from) || is_wildcard(to)) return true;  // subgraph
    return from == to;                                      // outlets etc.
}

// "" → true edge (transparent, same region). A name → the canonical
// mapping the executor must insert. nullopt → illegal connection.
constexpr std::optional<std::string_view>
boundary_mapping(std::string_view from, std::string_view to) {
    if (!connection_legal(from, to)) return std::nullopt;
    if (is_wildcard(from) || is_wildcard(to)) return "";
    Rate rf = rate_of(from), rt = rate_of(to);
    if (rf == rt) return "";
    if (rf == Rate::Frame && rt == Rate::Block) return "latch";
    if (rf == Rate::Block && rt == Rate::Frame) return "snapshot";
    return "queue";  // event boundaries (placeholder until step 3)
}

} // namespace port_types
