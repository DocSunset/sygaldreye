// Copyright 2026 Travis West
#pragma once
#include "signal_graph.hpp"
#include <deque>
#include <optional>

// Edge executor step 2 (planning/edge_executor.design.md): true edges are
// references resolved once per graph, not per-tick string lookups; edges
// that close cycles are reified as z⁻¹ delay mappings — the certified
// one-tick feedback behavior, by construction.

// One resolved connection: a slot in Graph::values → the target node.
// Resolution is lazy (the slot exists once the producer first emits),
// then cached. Slot addresses are stable: unordered_map never erases here.
struct EdgeApplier {
    const Edge*      edge;
    const PortValue* src = nullptr;
};

// A cycle-closing edge: applies last tick's captured value (z⁻¹).
// prev stays empty until the first capture, so tick 1 behaves exactly as
// an unconnected input — the certified feedback semantics.
struct DelayMapping {
    EdgeApplier              applier;
    std::optional<PortValue> prev;
};

struct TickPlan {
    std::vector<std::size_t>                 order;     // DAG order, cycles broken at delays
    std::vector<std::vector<EdgeApplier>>    appliers;  // true edges, per target node index
    std::vector<std::vector<DelayMapping*>>  delayed;   // per target node index
    std::deque<DelayMapping>                 delays;    // owned; deque = stable addresses
};

// Builds the plan: DFS marks back edges (the z⁻¹ set), Kahn orders the rest.
std::unique_ptr<TickPlan> build_plan(const Graph& g);

// Which edges are currently mediated by an auto-inserted z⁻¹? (editor /
// serialization visibility; recomputed, never persisted)
std::vector<const Edge*> cycle_mappings(const Graph& g);
