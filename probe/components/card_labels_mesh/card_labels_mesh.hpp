// Copyright 2026 Travis West
#pragma once
#include <string_view>

#include "editor_layout.hpp"
#include "render_payloads.hpp"
#include "sygaldry_endpoints.hpp"

// card_labels_mesh (vr_editor_decomposition.md S5): every card's id label as ONE
// glyph Mesh + the MSDF atlas Surface (the same glyph machinery text_label
// uses, batched across cards — text_label renders ONE string). Labels read off
// editor_layout's identity strings (the simplest identity resolution). A
// resource-holder: it observes the graph it lives in.
struct CardLabelsMeshNode {
    static consteval std::string_view name() { return "card_labels_mesh"; }
    static consteval std::string_view source_header() {
        return "components/card_labels_mesh/card_labels_mesh.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        normalled_in<float, fp(0.01f), fp(5.f), fp(0.5f)> scale;  // text size
        ::out<Mesh> mesh;
        ::out<Surface> surface;
    } endpoints;

    void operator()(double);
    void set_context(const editor_layout::GestureContext& ctx) { ctx_ = ctx; }

private:
    editor_layout::GestureContext ctx_;
    Shader shader_;
};
