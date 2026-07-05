// Copyright 2026 Travis West
#pragma once
#include <string_view>
#include <vector>

#include "editor_layout.hpp"
#include "sygaldry_endpoints.hpp"

// editor_wires (vr_editor_decomposition.md, from VrEditor::collect_wires): the
// live graph's edges as an N×10 wire Span {from.xyz, to.xyz, rgba} for a
// wire_mesh node, coloured by the producer port's kind. Reads the same
// editor_layout the cards and gestures do, so a wire lands on the handles. A
// resource-holder: it observes the graph it lives in.
struct EditorWiresNode {
    static consteval std::string_view name() { return "editor_wires"; }
    static consteval std::string_view source_header() {
        return "components/editor_wires/editor_wires.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        ::out<Span> wires;  // N×10
    } endpoints;

    void operator()(double);
    void set_context(const editor_layout::GestureContext& ctx) { ctx_ = ctx; }

private:
    editor_layout::GestureContext ctx_;
    std::vector<float> rows_;
};
