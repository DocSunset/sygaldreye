// Copyright 2025 Travis West
// Internal helpers shared across vr_editor implementation files.
#pragma once
#include <tuple>

#include "port_schema_reader.hpp"
#include "vr_editor.hpp"

// Color by port kind
inline Eigen::Vector4f port_kind_color(const std::string& kind) {
    if (kind == "scalar") return {0.2f, 0.4f, 1.0f, 1.0f};     // blue
    if (kind == "vec3") return {0.2f, 0.9f, 0.3f, 1.0f};       // green
    if (kind == "vec4") return {0.2f, 0.9f, 0.9f, 1.0f};       // cyan
    if (kind == "texture") return {1.0f, 0.5f, 0.1f, 1.0f};    // orange
    if (kind == "audio") return {1.0f, 0.9f, 0.1f, 1.0f};      // yellow
    if (kind == "draw_call") return {0.8f, 0.3f, 0.8f, 1.0f};  // purple
    return {0.5f, 0.5f, 0.5f, 1.0f};                           // grey
}

// Build port handles and sliders for one card.
// Returns (input_handles, output_handles, sliders).
std::tuple<std::vector<PortHandle>, std::vector<PortHandle>, std::vector<SliderWidget>>
build_port_handles(const VrPanel& card, const std::string& node_id, const PortSchema& schema);

// Draw a filled quad at world_pos with given half-extents using RgbaShader.
void draw_quad(
    const Eigen::Vector3f& pos,
    float hw,
    float hh,
    const Eigen::Vector4f& color,
    const Eigen::Matrix4f& vp,
    RgbaShader& shader);

// VrEditor sub-update helpers (defined in vr_editor_interactions.cpp)
std::optional<VrEditor::GraphEdit> VrEditor_update_drag(
    VrEditor& self, bool grip_right, const Graph*);
std::optional<VrEditor::GraphEdit> VrEditor_update_sliders(
    VrEditor& self, const XrPosef*, bool trigger_right, const Graph*);
std::optional<VrEditor::GraphEdit> VrEditor_update_dwell(
    VrEditor& self, const XrPosef*, float dt, const Graph*);
std::optional<VrEditor::GraphEdit> VrEditor_update_undo(
    VrEditor& self, const Eigen::Vector2f& thumb);
