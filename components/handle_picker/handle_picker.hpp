// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#include <string_view>

#include "editor_layout.hpp"
#include "sygaldry_endpoints.hpp"

// handle_picker (vr_editor_decomposition.md S3, from vr_editor.cpp hover loop):
// the nearest port handle / slider to the tip → a legible hover label + its
// world position, for a text_label to render. Output ports (not edit ops):
// hover is feedback, not an edit. A resource-holder: it observes the graph it
// lives in. (Labels read off the layout's identity strings — the simplest
// identity resolution, no text-array payload.)
struct HandlePickerNode {
    static consteval std::string_view name() { return "handle_picker"; }
    static consteval std::string_view source_header() {
        return "components/handle_picker/handle_picker.hpp";
    }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        ::in<Eigen::Vector3f> pos;
        ::in<Eigen::Quaternionf> rot;
        ::out<std::string> label;        // "" when nothing is near
        ::out<Eigen::Vector3f> pos_out;  // label anchor (world)
        // Anchor as scalars too: text_label takes pos_x/y/z, not a vec3.
        ::out<float> pos_x;
        ::out<float> pos_y;
        ::out<float> pos_z;
    } endpoints;

    void operator()(double);
    void set_context(const editor_layout::GestureContext& ctx) { ctx_ = ctx; }
    void set_host_context(const char* kind, void* ctx) {
        if (std::string_view{kind} == editor_layout::kEditorContextKind)
            set_context(*static_cast<const editor_layout::GestureContext*>(ctx));
    }

private:
    editor_layout::GestureContext ctx_;
};
