// Copyright 2026 Travis West
#pragma once
#include <memory>
#include <string_view>

#include <Eigen/Core>

#include "eyeballs_node_abi.hpp"
#include "render_payloads.hpp"

// vertex_color_mesh: a shader-specific node for lit, per-vertex-colored
// geometry (terrain, reaction-diffusion grids, editor quads). The mesh carries
// its colors in aColor; this node applies one directional sun (Blinn-Phong)
// and emits Surface + Mesh. dynamic is a usage hint (producers animate the
// geometry); staleness is the producer's TriMeshData version — render_region
// re-uploads only when it changes. Sun is wireable (sky.sun_dir/sun_color).
namespace vertex_color_mesh_detail {
inline constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
out vec3 vNormal;
out vec4 vColor;
out vec3 vFragPos;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vFragPos = aPos;       // geometry is already world-space (no model matrix)
    vNormal  = aNormal;
    vColor   = aColor;
}
)";
inline constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec3 vNormal;
in vec4 vColor;
in vec3 vFragPos;
uniform vec3  uViewPos;
uniform vec3  uSunDir;
uniform vec3  uSunColor;
uniform float uSunIntensity;
out vec4 fragColor;
const float kAmbient   = 0.30;
const float kSpecular  = 0.18;
const float kShininess = 16.0;
void main() {
    vec3  norm     = normalize(vNormal);
    vec3  lightDir = normalize(-uSunDir);
    vec3  viewDir  = normalize(uViewPos - vFragPos);
    vec3  halfDir  = normalize(lightDir + viewDir);
    float diff     = max(dot(norm, lightDir), 0.0);
    float spec     = pow(max(dot(norm, halfDir), 0.0), kShininess);
    vec3  sun      = uSunColor * uSunIntensity;
    vec3  base     = vColor.rgb;
    vec3  result   = base * kAmbient + base * sun * diff + vec3(kSpecular) * sun * spec;
    fragColor = vec4(result, vColor.a);
}
)";
}  // namespace vertex_color_mesh_detail

struct VertexColorMeshNode {
    static consteval std::string_view name() { return "vertex_color_mesh"; }
    static consteval std::string_view source_header() {
        return "components/vertex_color_mesh/vertex_color_mesh.hpp";
    }
    struct endpoints {
        ::in<MeshPtr>         geometry;
        ::in<Eigen::Vector3f> sun_dir;    // unwired ⇒ default downward sun
        ::in<Eigen::Vector3f> sun_color;  // unwired ⇒ warm white
        normalled_in<float, fp(0.f), fp(5.f), fp(1.1f)> sun_intensity;
        ::out<Surface>        surface;
        ::out<Mesh>           mesh;
    } endpoints;
    void operator()(double) {
        if (!shader_)
            shader_ = std::make_shared<ShaderData>(ShaderData{
                vertex_color_mesh_detail::kVert, vertex_color_mesh_detail::kFrag});
        Mesh m;
        m.geometry = endpoints.geometry.get();
        m.mode     = Primitive::Triangles;
        m.dynamic  = true;  // usage hint: producers (terrain/RD/editor) animate it
        endpoints.mesh.value = std::move(m);

        const Eigen::Vector3f dir =
            endpoints.sun_dir.src ? *endpoints.sun_dir.src : Eigen::Vector3f(-0.4f, -0.9f, -0.2f);
        const Eigen::Vector3f col =
            endpoints.sun_color.src ? *endpoints.sun_color.src : Eigen::Vector3f(1.0f, 0.95f, 0.85f);
        Surface s;
        s.shader = shader_;
        s.uniforms.push_back({"uSunDir", dir});
        s.uniforms.push_back({"uSunColor", col});
        s.uniforms.push_back({"uSunIntensity", endpoints.sun_intensity.get()});
        endpoints.surface.value = std::move(s);
    }

   private:
    Shader shader_;
};
