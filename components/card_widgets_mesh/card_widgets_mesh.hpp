// Copyright 2026 Travis West
#pragma once
#include <memory>
#include <string_view>

#include "editor_layout.hpp"
#include "render_payloads.hpp"
#include "sygaldry_endpoints.hpp"

// card_widgets_mesh (vr_editor_decomposition.md S5): the card INTERNALS as one
// vertex-colored geometry — every card's per-port handles (colored by kind,
// inputs left / outputs right) and scalar sliders (track + thumb). The
// rendering dual of the gesture nodes: both read editor_layout off the graph,
// so a handle drawn here is exactly where wire_drag hit-tests. Emits a MeshPtr
// (geometry only); wire it into a vertex_color_mesh for the Surface, then draw.
// A resource-holder: it observes the graph it lives in.
struct CardWidgetsMeshNode {
    static consteval std::string_view name() { return "card_widgets_mesh"; }
    static consteval std::string_view source_header() {
        return "components/card_widgets_mesh/card_widgets_mesh.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        ::out<MeshPtr> mesh;
    } endpoints;

    void operator()(double);
    void set_context(const editor_layout::GestureContext& ctx) { ctx_ = ctx; }
    void set_host_context(const char* kind, void* ctx) {
        if (std::string_view{kind} == editor_layout::kEditorContextKind)
            set_context(*static_cast<const editor_layout::GestureContext*>(ctx));
    }

private:
    editor_layout::GestureContext ctx_;
    MeshPtr cached_;
};
