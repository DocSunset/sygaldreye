// Copyright 2025 Travis West
#include "cube_node.hpp"

#include <cmath>
#include <memory>

#include "tri_mesh.hpp"

namespace {

constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uMVP;
out vec3 vNormal;
out vec3 vFragPos;
void main() {
    vNormal = aNormal;
    vFragPos = aPos;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec3 vNormal;
in vec3 vFragPos;
uniform vec3  uViewPos;
uniform vec4  uColor;
uniform vec3  uSunDir;
uniform vec3  uSunColor;
uniform float uSunIntensity;
uniform float uShininess;
out vec4 fragColor;
void main() {
    vec3  n  = normalize(vNormal);
    vec3  l  = normalize(-uSunDir);
    vec3  v  = normalize(uViewPos - vFragPos);
    vec3  h  = normalize(l + v);
    float d  = max(dot(n, l), 0.0);
    float sp = pow(max(dot(n, h), 0.0), uShininess);
    vec3  sun = uSunColor * uSunIntensity;
    vec3  res = uColor.rgb * 0.25 + uColor.rgb * sun * d + vec3(0.2) * sun * sp;
    fragColor = vec4(res, uColor.a);
}
)";

// Unit box centered at origin, scaled by s and translated by c (baked in).
MeshPtr make_box(const Eigen::Vector3f& c, float s) {
    auto                  m    = std::make_shared<TriMeshData>();
    const Eigen::Vector3f n[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0},
                                  {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (int f = 0; f < 6; ++f) {
        Eigen::Vector3f u    = (f < 2) ? Eigen::Vector3f{0, 1, 0} : Eigen::Vector3f{1, 0, 0};
        Eigen::Vector3f v    = n[f].cross(u);
        uint32_t        base = uint32_t(m->vertices.size());
        for (int k = 0; k < 4; ++k) {
            float           du = (k == 1 || k == 2) ? 1.f : -1.f;
            float           dv = (k >= 2) ? 1.f : -1.f;
            Eigen::Vector3f p  = c + (n[f] + u * du + v * dv) * (0.5f * s);
            m->vertices.push_back({p, n[f], {1, 1, 1, 1}});
        }
        m->indices.insert(m->indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
    }
    return m;
}

}  // namespace

void CubeNode::operator()(double /*time_s*/) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});

    const Eigen::Vector3f pos{endpoints.pos_x.get(), endpoints.pos_y.get(), endpoints.pos_z.get()};
    Mesh                  m;
    m.geometry = make_box(pos, endpoints.scale.get());
    m.dynamic  = true;  // pos/scale baked; rebuilt on change
    endpoints.mesh.value = std::move(m);

    Eigen::Vector3f ldir = endpoints.light_dir.get();
    float           lint = endpoints.light_intensity.get();
    Eigen::Vector3f lcol =
        endpoints.light_color.src ? *endpoints.light_color.src : Eigen::Vector3f::Ones();
    if (lint == 0.f && ldir.squaredNorm() == 0.f) {  // unset → default downward sun
        ldir = {0.f, -1.f, 0.f};
        lint = 1.0f;
    }
    const float r         = endpoints.roughness.get();
    const float shininess = 2.f / (r * r + 1e-4f) - 1.f;

    Surface s;
    s.shader = shader_;
    s.uniforms.push_back({"uColor", Eigen::Vector4f(endpoints.color_r.get(), endpoints.color_g.get(),
                                                    endpoints.color_b.get(), 1.0f)});
    s.uniforms.push_back({"uSunDir", ldir});
    s.uniforms.push_back({"uSunColor", lcol});
    s.uniforms.push_back({"uSunIntensity", lint});
    s.uniforms.push_back({"uShininess", shininess});
    endpoints.surface.value = std::move(s);
}
