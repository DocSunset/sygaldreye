// Copyright 2026 Travis West
#pragma once
#include <string>
#include <string_view>

#include "editor_layout.hpp"
#include "sygaldry_endpoints.hpp"

// undo_node (vr_editor_decomposition.md S3, from update_undo): a left-thumb
// flick (thumb_x < -0.7) restores the graph as it was before the last edit. It
// keeps a 1-deep snapshot of the live graph, capturing a NEW baseline whenever
// the graph changes; the flick re-posts the prior snapshot as a whole-graph
// edit. A resource-holder: it observes the graph it lives in.
struct UndoNode {
    static consteval std::string_view name() { return "undo_node"; }
    static consteval std::string_view source_header() {
        return "components/undo_node/undo_node.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        ::in<float> thumb_x;  // left thumbstick x
    } endpoints;

    void operator()(double);
    void set_context(const editor_layout::GestureContext& ctx) { ctx_ = ctx; }

private:
    editor_layout::GestureContext ctx_;
    bool prev_gesture_ = false;
    std::string current_sig_;  // structural signature of the current graph
    std::string current_;      // current graph JSON (latest params)
    std::string previous_;     // the graph before the last STRUCTURAL edit
};
