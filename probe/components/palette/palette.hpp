// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string_view>
#include <vector>

#include "editor_layout.hpp"
#include "sygaldry_endpoints.hpp"

// palette (vr_editor_decomposition.md S3, from vr_editor.cpp palette block):
// a poke (trigger rising edge with the tip in the panel) on a type row emits an
// `add_node` op; row 0 flips pages. The type list comes from the registry via
// the gesture context. A resource-holder: it observes the graph it lives in.
struct PaletteNode {
    static consteval std::string_view name() { return "palette"; }
    static consteval std::string_view source_header() { return "components/palette/palette.hpp"; }
    static constexpr int lift_kind() { return lift::resource_holder; }

    static constexpr int kRows = 15;
    static Eigen::Vector3f panel_pos() { return {-0.55f, 1.2f, -0.5f}; }
    static constexpr float kPaletteRowH = 0.06f;
    static constexpr float panel_w() { return 0.35f; }
    static float panel_h() { return kPaletteRowH * float(kRows + 1); }

    struct endpoints {
        ::in<Eigen::Vector3f> pos;
        ::in<Eigen::Quaternionf> rot;
        ::in<float> trigger;
    } endpoints;

    void operator()(double);
    void set_context(const editor_layout::GestureContext& ctx) { ctx_ = ctx; }
    int page() const { return page_; }

private:
    editor_layout::GestureContext ctx_;
    bool prev_trigger_ = false;
    int page_ = 0;
    int pages() const {
        int n = ctx_.types ? int(ctx_.types->size()) : 0;
        return n > 0 ? (n + kRows - 1) / kRows : 1;
    }
};
