// Copyright 2026 Travis West
#pragma once
#include <string_view>

#include "editor_layout.hpp"
#include "render_payloads.hpp"
#include "sygaldry_endpoints.hpp"

// palette_mesh (vr_editor_decomposition.md S5): the node palette as geometry —
// the backdrop panel + the current page's type-row labels (header row +
// kRows). The render dual of the palette gesture node; both use the same panel
// position/paging so a poke lands on the row drawn here. Emits the panel as a
// vertex-color Mesh and the labels as a glyph Mesh + atlas Surface. A
// resource-holder: it reads the registry type list via the gesture context.
struct PaletteMeshNode {
    static consteval std::string_view name() { return "palette_mesh"; }
    static consteval std::string_view source_header() {
        return "components/palette_mesh/palette_mesh.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        ::in<float> page;   // current page (from a palette node, optional)
        ::out<Mesh> panel;  // backdrop (vertex-color)
        ::out<Surface> panel_surface;
        ::out<Mesh> labels;  // type-row glyphs
        ::out<Surface> labels_surface;
    } endpoints;

    void operator()(double);
    void set_context(const editor_layout::GestureContext& ctx) { ctx_ = ctx; }

private:
    editor_layout::GestureContext ctx_;
    Shader panel_shader_, text_shader_;
};
