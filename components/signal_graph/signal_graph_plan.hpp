// Copyright 2026 Travis West
#pragma once
#include "signal_graph.hpp"
#include "port_types.hpp"
#include <deque>
#include <optional>

// Edge executor (planning/edge_executor.design.md): ONE plan for the whole
// graph, ticked by per-region schedulers. True edges are references
// resolved once against the owning region's value store; edges that close
// cycles are reified z⁻¹ delay mappings; edges that cross regions are
// reified boundary mappings chosen by port_types::crossing_mapping.
//
// Regions (v1): Block = `dac` nodes + upstream closure through audio
// edges, ticked by the audio scheduler; everything else is Frame (render).
// (The pure rate-quotient inference from the design is deliberately
// narrowed: a node with audio IN but no audio OUT — spectrogram — pins
// render via its other ports and consumes the stream through a ring.)

// One resolved connection: a slot in the owning region's store → the
// target node. Resolution is lazy, then cached (slots are stable).
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
    port_types::Rate         region = port_types::Rate::Frame;
};

// A region-crossing edge, to be mediated by the named canonical mapping
// (ring / latch / snapshot / queue). The audio scheduler instantiates
// the mapping state; the plan only declares it.
struct Crossing {
    const Edge*      edge;
    std::string      payload_kind;
    std::string_view mapping;
    port_types::Rate from_region, to_region;
};

struct TickPlan {
    std::vector<port_types::Rate>            node_region;   // per node index
    std::vector<std::size_t>                 order;         // Frame, DAG order
    std::vector<std::size_t>                 block_order;   // Block, DAG order
    std::vector<std::vector<EdgeApplier>>    appliers;      // same-region true edges
    std::vector<std::vector<DelayMapping*>>  delayed;       // per target node
    std::deque<DelayMapping>                 delays;        // owned; stable addresses
    std::vector<Crossing>                    crossings;
};

std::unique_ptr<TickPlan> build_plan(const Graph& g);

// Which edges are currently mediated by an auto-inserted z⁻¹? (editor /
// serialization visibility; recomputed, never persisted)
std::vector<const Edge*> cycle_mappings(const Graph& g);

// Shared executor primitives (one implementation for every scheduler —
// render, block, subgraph, net proxies).
void apply_value(const NodeInstance& n, const char* port, const PortValue& value);
// Output context whose emit callbacks write into `store`.
EyeballsOutputCtx output_ctx(std::unordered_map<std::string, PortValue>* store,
                             const char* node_id);
// Lazy by-ref resolution of an applier against a region's store.
const PortValue* resolve_applier(EdgeApplier& a,
                                 const std::unordered_map<std::string, PortValue>& store);
