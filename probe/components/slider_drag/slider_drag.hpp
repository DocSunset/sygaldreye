// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string_view>

#include "editor_layout.hpp"
#include "sygaldry_endpoints.hpp"

// slider_drag (vr_editor_decomposition.md S3, from update_sliders +
// vr_editor_handles slider layout): the tip's x over the single nearest slider
// track → a normalized value in [min,max] → a `set_param` edit op, while the
// trigger is held. One slider at a time (rows are 0.018 m apart). A
// resource-holder: it observes the graph it lives in.
struct SliderDragNode {
    static consteval std::string_view name() { return "slider_drag"; }
    static consteval std::string_view source_header() {
        return "components/slider_drag/slider_drag.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        ::in<Eigen::Vector3f> pos;
        ::in<Eigen::Quaternionf> rot;
        ::in<float> trigger;
    } endpoints;

    void operator()(double);
    void set_context(const editor_layout::GestureContext& ctx) { ctx_ = ctx; }

private:
    editor_layout::GestureContext ctx_;
};
