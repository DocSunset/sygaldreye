// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "signal_graph.hpp"
#include "sygaldry_endpoints.hpp"

// The editor's meta seam (vr_editor_decomposition.md): the ONE node that
// reads the enclosing graph and publishes it as data for the rest of the
// editor (a lifted card-subgraph) to consume. Replaces EditorNode's raw
// set_context pointer injection — the editor stops being a monolith and
// becomes graph content fed by this source.
//
// A resource-holder: it reads the graph it lives in, so it can't be lifted
// (and the enclosing graph can't be a port — set_context injects it each
// frame, exactly like the editor/spawner seam it replaces).
//
// Outputs the graph model as Spans keyed positionally:
//   keys      — N×1 stable per-node id hash (the KEYED-LIFT key source: a
//               card subgraph lifted over `positions` keys on THIS, not the
//               position cell, so a dragged card keeps its identity).
//   positions — N×3 card world positions (the lifted-cell source → card.pos).
//   count     — scalar N (node count), for sinks that need the row count.
struct GraphSourceNode {
    static consteval std::string_view name() { return "graph_source"; }
    static consteval std::string_view source_header() {
        return "components/graph_source/graph_source.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        out<Span> keys;       // N×1 id hashes
        out<Span> positions;  // N×3 card positions
        out<float> count;     // N
    } endpoints;

    void operator()(double time_s);

    // The shell points us at the live graph AND the shared card-position
    // overrides (wire_drag writes them; we publish positions from them) so a
    // dragged card stays where it was dropped.
    void set_context(
        const Graph* g, const std::unordered_map<std::string, Eigen::Vector3f>* ovr = nullptr) {
        graph_ = g;
        pos_ovr_ = ovr;
    }

private:
    const Graph* graph_ = nullptr;
    std::vector<float> keys_;
    std::vector<float> pos_;
    const std::unordered_map<std::string, Eigen::Vector3f>* pos_ovr_ = nullptr;  // dragged cards
};
