// Copyright 2026 Travis West
#include "card_widgets_mesh.hpp"

#include <algorithm>

#include "tri_mesh.hpp"

namespace {
// Handle colour by port kind (ported from vr_editor_handles port_kind_color).
Eigen::Vector4f kind_color(const std::string& kind) {
    if (kind == "scalar") return {0.2f, 0.4f, 1.0f, 1.0f};
    if (kind == "vec3") return {0.2f, 0.9f, 0.3f, 1.0f};
    if (kind == "vec4") return {0.2f, 0.9f, 0.9f, 1.0f};
    if (kind == "texture") return {1.0f, 0.5f, 0.1f, 1.0f};
    if (kind == "audio") return {1.0f, 0.9f, 0.1f, 1.0f};
    if (kind == "mesh" || kind == "surface" || kind == "drawable") return {0.8f, 0.3f, 0.8f, 1.0f};
    return {0.5f, 0.5f, 0.5f, 1.0f};
}

// Two triangles (six verts) for an axis-aligned quad facing +z, appended to m.
void quad(
    TriMeshData& m, const Eigen::Vector3f& c, float hw, float hh, const Eigen::Vector4f& col) {
    const Eigen::Vector3f n{0, 0, 1};
    const float dx[4] = {-hw, hw, hw, -hw};
    const float dy[4] = {-hh, -hh, hh, hh};
    auto base = std::uint32_t(m.vertices.size());
    for (int i = 0; i < 4; ++i)
        m.vertices.push_back({{c.x() + dx[i], c.y() + dy[i], c.z()}, n, col});
    for (std::uint32_t i : {0u, 1u, 2u, 0u, 2u, 3u}) m.indices.push_back(base + i);
}
}  // namespace

void CardWidgetsMeshNode::operator()(double) {
    if (!ctx_.graph) {
        endpoints.mesh.value = nullptr;
        return;
    }
    const editor_layout::Layout& l = editor_layout::cached_layout(ctx_);

    auto m = std::make_shared<TriMeshData>();
    constexpr float hs = editor_layout::kHandleHalf;  // 0.006
    for (const auto& card : l.cards) {
        for (const auto& h : card.inputs) quad(*m, h.world_pos, hs, hs, kind_color(h.port_kind));
        for (const auto& h : card.outputs) quad(*m, h.world_pos, hs, hs, kind_color(h.port_kind));
        for (const auto& sl : card.sliders) {
            // Track (dark) + thumb (light blue) at the value position.
            quad(*m, sl.world_pos, sl.width * 0.5f, 0.004f, {0.2f, 0.2f, 0.2f, 1.0f});
            float norm = (sl.max_val > sl.min_val)
                             ? (sl.value - sl.min_val) / (sl.max_val - sl.min_val)
                             : 0.5f;
            norm = std::clamp(norm, 0.f, 1.f);
            float tx = sl.world_pos.x() - sl.width * 0.5f + norm * sl.width;
            quad(
                *m,
                {tx, sl.world_pos.y(), sl.world_pos.z() + 0.004f},
                0.004f,
                0.005f,
                {0.7f, 0.8f, 1.0f, 1.0f});
        }
    }
    cached_ = m;
    endpoints.mesh.value = cached_;
}
