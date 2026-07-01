// Copyright 2026 Travis West
#include "ui_nodes.hpp"

#include <algorithm>
#include <memory>
#include <optional>

#include "tri_mesh.hpp"

namespace {

// Unlit, per-vertex-color program: lets a single mesh carry differently
// colored quads (slider track + thumb) in one draw. render_region injects
// uMVP. Blends so panels read as translucent glass.
constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
out vec4 vColor;
void main() { vColor = aColor; gl_Position = uMVP * vec4(aPos, 1.0); }
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() { fragColor = vColor; }
)";

Eigen::Vector3f ray_dir(const Eigen::Quaternionf& q_in) {
    Eigen::Quaternionf q =
        (q_in.norm() > 1e-6f) ? q_in.normalized() : Eigen::Quaternionf::Identity();
    return q * Eigen::Vector3f{0.f, 0.f, -1.f};
}

// Ray vs +Z-facing panel at (cx,cy,cz), size w×h. Returns uv in [0,1]^2 when
// the hit lands on the quad. (Replaces VrPanel::intersect; same math.)
std::optional<Eigen::Vector2f> panel_hit(
    float cx,
    float cy,
    float cz,
    float w,
    float h,
    const Eigen::Vector3f& rp,
    const Eigen::Vector3f& rd) {
    if (std::abs(rd.z()) < 1e-6f) return std::nullopt;
    float t = (cz - rp.z()) / rd.z();
    if (t < 0.f) return std::nullopt;
    Eigen::Vector3f hit = rp + t * rd;
    float u = (hit.x() - cx) / w + 0.5f;
    float v = (hit.y() - cy) / h + 0.5f;
    if (u < 0.f || u > 1.f || v < 0.f || v > 1.f) return std::nullopt;
    return Eigen::Vector2f{u, v};
}

// Append a +Z-facing quad (centered cx,cy,cz, size w×h) of one color.
void add_quad(
    TriMeshData& m, float cx, float cy, float cz, float w, float h, const Eigen::Vector4f& color) {
    const float hw = w * 0.5f, hh = h * 0.5f;
    const Eigen::Vector3f nz{0.f, 0.f, 1.f};
    uint32_t base = uint32_t(m.vertices.size());
    const Eigen::Vector3f c[4] = {
        {cx - hw, cy - hh, cz},
        {cx + hw, cy - hh, cz},
        {cx + hw, cy + hh, cz},
        {cx - hw, cy + hh, cz}};
    for (auto& p : c) m.vertices.push_back({p, nz, color});
    m.indices.insert(m.indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

Surface ui_surface(const Shader& shader) {
    Surface s;
    s.shader = shader;
    s.blend = true;
    s.depth_write = false;
    s.cull_back = false;
    return s;
}

}  // namespace

void UiSliderNode::operator()(double) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});
    float lo = endpoints.min.get(), hi = endpoints.max.get();
    if (value_norm_ < 0.f) {
        float span = hi - lo;
        value_norm_ =
            (span != 0.f) ? std::clamp((endpoints.init.get() - lo) / span, 0.f, 1.f) : 0.f;
    }

    float cx = endpoints.x.get(), cy = endpoints.y.get(), cz = endpoints.z.get();
    float w = endpoints.width.get();
    auto hit =
        panel_hit(cx, cy, cz, w, 0.03f, endpoints.ray_pos.get(), ray_dir(endpoints.ray_rot.get()));
    hover_ = hit.has_value();
    if (hover_ && endpoints.trigger.get() > 0.5f) value_norm_ = std::clamp(hit->x(), 0.f, 1.f);

    endpoints.value.value = lo + value_norm_ * (hi - lo);

    auto mesh = std::make_shared<TriMeshData>();
    add_quad(*mesh, cx, cy, cz, w, 0.03f, {0.15f, 0.17f, 0.22f, 0.95f});  // track
    Eigen::Vector4f thumb_c =
        hover_ ? Eigen::Vector4f{0.6f, 0.9f, 1.f, 1.f} : Eigen::Vector4f{0.4f, 0.6f, 0.9f, 1.f};
    add_quad(*mesh, cx + (value_norm_ - 0.5f) * w, cy, cz + 0.005f, 0.02f, 0.05f, thumb_c);

    Mesh m;
    m.geometry = mesh;
    m.dynamic = true;
    endpoints.mesh.value = std::move(m);
    endpoints.surface.value = ui_surface(shader_);
}

void UiButtonNode::operator()(double) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});
    float cx = endpoints.x.get(), cy = endpoints.y.get(), cz = endpoints.z.get();
    float w = endpoints.width.get(), h = endpoints.height.get();
    auto hit =
        panel_hit(cx, cy, cz, w, h, endpoints.ray_pos.get(), ray_dir(endpoints.ray_rot.get()));
    hover_ = hit.has_value();
    pressed_ = hover_ && endpoints.trigger.get() > 0.5f;
    endpoints.pressed.value = pressed_ ? 1.f : 0.f;
    endpoints.hover.value = hover_ ? 1.f : 0.f;

    Eigen::Vector4f color = pressed_ ? Eigen::Vector4f{0.9f, 0.6f, 0.2f, 1.f}
                            : hover_ ? Eigen::Vector4f{0.35f, 0.45f, 0.55f, 1.f}
                                     : Eigen::Vector4f{0.2f, 0.25f, 0.3f, 0.95f};
    auto mesh = std::make_shared<TriMeshData>();
    add_quad(*mesh, cx, cy, cz, w, h, color);

    Mesh m;
    m.geometry = mesh;
    m.dynamic = true;
    endpoints.mesh.value = std::move(m);
    endpoints.surface.value = ui_surface(shader_);
}

void UiPaneNode::operator()(double) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});
    auto mesh = std::make_shared<TriMeshData>();
    add_quad(
        *mesh,
        endpoints.x.get(),
        endpoints.y.get(),
        endpoints.z.get(),
        endpoints.width.get(),
        endpoints.height.get(),
        {endpoints.r.get(), endpoints.g.get(), endpoints.b.get(), endpoints.alpha.get()});

    Mesh m;
    m.geometry = mesh;
    m.dynamic = true;
    endpoints.mesh.value = std::move(m);
    endpoints.surface.value = ui_surface(shader_);
}

#ifdef SYGALDREYE_PLUGIN
#include "eyeballs_node_abi.hpp"
EYEBALLS_EXPORT_NODE(UiSliderNode)
#endif
