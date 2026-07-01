// Copyright 2026 Travis West
#include "aurora_curtain.hpp"

#include <memory>

#include "tri_mesh.hpp"

namespace {
// Ripple math unchanged; aPos.xy carries the UV grid (z unused). uMVP + uTime
// injected by render_region.
constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
uniform mat4  uMVP;
uniform float uTime, uXOffset, uPhase, uFreq, uSpeed;
uniform float uRippleAmp, uAltBase, uAltHeight, uDepthExtent;
uniform vec3  uColor;
out float vAlpha;
out vec3  vColor;
void main() {
    float u = aPos.x, v = aPos.y;
    float z = (u - 0.5) * uDepthExtent;
    float bottom_weight = 1.0 - v * 0.8;
    float w1 = sin(u * uFreq * 4.712 + uTime * uSpeed        + uPhase);
    float w2 = sin(u * uFreq * 7.854 - uTime * uSpeed * 0.61 + uPhase * 1.4);
    float w3 = sin(u * uFreq * 2.618 + uTime * uSpeed * 1.37 + uPhase * 0.5);
    float ripple = (w1 * 0.5 + w2 * 0.3 + w3 * 0.2) * uRippleAmp * bottom_weight;
    float sway = sin(uTime * uSpeed * 0.28 + uPhase * 0.8) * uRippleAmp * 0.25;
    gl_Position = uMVP * vec4(uXOffset + ripple + sway, uAltBase + v * uAltHeight, z, 1.0);
    float bright = sin(u * uFreq * 3.5 + uTime * uSpeed * 0.9 + uPhase) * 0.25 + 0.75;
    float pulse  = sin(uTime * uSpeed * 0.4 + uPhase * 1.1) * 0.12 + 0.88;
    vAlpha = clamp(v * v * bright * pulse * 0.9, 0.0, 1.0);
    vColor = uColor;
}
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in float vAlpha;
in vec3  vColor;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, vAlpha); }
)";
constexpr int kSegW = 60, kSegH = 24;

MeshPtr make_grid() {
    auto m = std::make_shared<TriMeshData>();
    m->vertices.reserve(static_cast<size_t>(kSegW + 1) * (kSegH + 1));
    for (int j = 0; j <= kSegH; ++j)
        for (int i = 0; i <= kSegW; ++i) {
            TriVertex v;
            v.position = Eigen::Vector3f(float(i) / kSegW, float(j) / kSegH, 0.f);  // UV in xy
            v.normal   = Eigen::Vector3f(0.f, 0.f, 1.f);
            v.color    = Eigen::Vector4f(1.f, 1.f, 1.f, 1.f);
            m->vertices.push_back(v);
        }
    m->indices.reserve(static_cast<size_t>(kSegW) * kSegH * 6);
    for (int j = 0; j < kSegH; ++j)
        for (int i = 0; i < kSegW; ++i) {
            uint32_t base = uint32_t(j * (kSegW + 1) + i), w = kSegW + 1;
            m->indices.insert(m->indices.end(),
                              {base, base + w, base + 1, base + 1, base + w, base + w + 1});
        }
    return m;
}
}  // namespace

void AuroraCurtainNode::operator()(double /*time_s*/) {
    if (!grid_) grid_ = make_grid();
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});

    Mesh m;
    m.geometry = grid_;
    m.mode     = Primitive::Triangles;
    endpoints.mesh.value = std::move(m);

    Surface s;
    s.shader      = shader_;
    s.blend       = true;
    s.additive    = true;
    s.depth_write = false;  // glow; doesn't occlude
    s.cull_back   = false;
    s.uniforms.push_back({"uXOffset", endpoints.x_offset.get()});
    s.uniforms.push_back({"uPhase", endpoints.phase.get()});
    s.uniforms.push_back({"uFreq", endpoints.freq.get()});
    s.uniforms.push_back({"uSpeed", endpoints.speed.get()});
    s.uniforms.push_back({"uRippleAmp", endpoints.ripple_amp.get()});
    s.uniforms.push_back({"uAltBase", endpoints.alt_base.get()});
    s.uniforms.push_back({"uAltHeight", endpoints.alt_height.get()});
    s.uniforms.push_back({"uDepthExtent", endpoints.depth.get()});
    s.uniforms.push_back(
        {"uColor", Eigen::Vector3f(endpoints.r.get(), endpoints.g.get(), endpoints.b.get())});
    endpoints.surface.value = std::move(s);
}
