// Copyright 2026 Travis West
#include "poke_button.hpp"

#include <cmath>
#include <memory>

#include "tri_mesh.hpp"

namespace {

// Unlit, uniform-color program. render_region injects uMVP; uColor encodes
// idle / hover / fire each tick with no geometry rebuild.
constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() { fragColor = uColor; }
)";

// Unit box centered at c, half-extent h, baked into world space.
MeshPtr make_box(const Eigen::Vector3f& c, float h) {
    auto m = std::make_shared<TriMeshData>();
    const Eigen::Vector3f n[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (int f = 0; f < 6; ++f) {
        Eigen::Vector3f u = (f < 2) ? Eigen::Vector3f{0, 1, 0} : Eigen::Vector3f{1, 0, 0};
        Eigen::Vector3f v = n[f].cross(u);
        uint32_t base = uint32_t(m->vertices.size());
        for (int k = 0; k < 4; ++k) {
            float du = (k == 1 || k == 2) ? 1.f : -1.f;
            float dv = (k >= 2) ? 1.f : -1.f;
            Eigen::Vector3f p = c + (n[f] + u * du + v * dv) * h;
            m->vertices.push_back({p, n[f], {1, 1, 1, 1}});
        }
        m->indices.insert(m->indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    return m;
}

}  // namespace

void PokeButtonNode::operator()(double) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});

    Eigen::Vector3f pos{endpoints.x.get(), endpoints.y.get(), endpoints.z.get()};
    float half = endpoints.size.get() * 0.5f;
    Eigen::Vector3f d = endpoints.tip.get() - pos;
    bool inside = std::abs(d.x()) < half && std::abs(d.y()) < half && std::abs(d.z()) < half;

    bool press = endpoints.press.get() > 0.5f;
    endpoints.pressed.triggered = inside && press && !prev_press_;
    prev_press_ = press;
    endpoints.hover.value = inside ? 1.f : 0.f;

    Mesh m;
    m.geometry = make_box(pos, half);
    m.dynamic = true;  // placement baked; rebuilt on change
    endpoints.mesh.value = std::move(m);

    bool lit = inside && press;
    Eigen::Vector4f color = lit      ? Eigen::Vector4f{1.f, 0.8f, 0.2f, 1.f}
                            : inside ? Eigen::Vector4f{0.4f, 0.6f, 1.f, 1.f}
                                     : Eigen::Vector4f{0.25f, 0.3f, 0.45f, 1.f};
    Surface s;
    s.shader = shader_;
    s.uniforms.push_back({"uColor", color});
    endpoints.surface.value = std::move(s);
}

#ifdef SYGALDREYE_PLUGIN
#include "eyeballs_node_abi.hpp"
EYEBALLS_EXPORT_NODE(PokeButtonNode)
#endif
