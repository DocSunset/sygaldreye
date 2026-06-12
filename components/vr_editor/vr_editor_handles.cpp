// Copyright 2025 Travis West
// Port handle layout + draw helpers
#include "vr_editor.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Geometry>
#include <array>

static constexpr float kHandleSize = 0.012f;
static constexpr float kHandleOffX = 0.01f;  // extra margin beyond card edge

Eigen::Vector4f port_kind_color(const std::string& kind) {
    if (kind == "scalar")    return {0.2f, 0.4f, 1.0f, 1.0f};
    if (kind == "vec3")      return {0.2f, 0.9f, 0.3f, 1.0f};
    if (kind == "vec4")      return {0.2f, 0.9f, 0.9f, 1.0f};
    if (kind == "texture")   return {1.0f, 0.5f, 0.1f, 1.0f};
    if (kind == "audio")     return {1.0f, 0.9f, 0.1f, 1.0f};
    if (kind == "draw_call") return {0.8f, 0.3f, 0.8f, 1.0f};
    return {0.5f, 0.5f, 0.5f, 1.0f};
}

// Build port handles for a card. Returns (inputs, outputs, sliders).
// Called from on_graph_changed.
std::tuple<std::vector<PortHandle>,
           std::vector<PortHandle>,
           std::vector<SliderWidget>>
build_port_handles_for_card(const VrPanel& card, const std::string& node_id,
                             const PortSchema& schema) {
    std::vector<PortHandle> ins, outs;
    std::vector<SliderWidget> sliders;

    float top = card.position.y() + card.height * 0.5f - kHandleSize;
    float left_x  = card.position.x() - card.width * 0.5f - kHandleOffX;
    float right_x = card.position.x() + card.width * 0.5f + kHandleOffX;
    float z = card.position.z() + 0.003f;

    int row = 0;
    for (const auto& p : schema.inputs) {
        if (!p.is_wirable()) continue;
        float y = top - static_cast<float>(row) * 0.018f;
        PortHandle h;
        h.node_id   = node_id;
        h.port_name = p.name;
        h.port_kind = p.kind;
        h.is_output = false;
        h.world_pos = {left_x, y, z};
        ins.push_back(h);

        if (p.kind == "scalar") {
            SliderWidget sl;
            sl.node_id   = node_id;
            sl.port_name = p.name;
            sl.world_pos = {card.position.x(), y, z};
            sl.width     = card.width * 0.6f;
            sl.min_val   = p.min;
            sl.max_val   = p.max;
            sliders.push_back(sl);
        }
        ++row;
    }

    row = 0;
    for (const auto& p : schema.outputs) {
        if (!p.is_wirable()) continue;
        float y = top - static_cast<float>(row) * 0.018f;
        PortHandle h;
        h.node_id   = node_id;
        h.port_name = p.name;
        h.port_kind = p.kind;
        h.is_output = true;
        h.world_pos = {right_x, y, z};
        outs.push_back(h);
        ++row;
    }

    return {std::move(ins), std::move(outs), std::move(sliders)};
}
