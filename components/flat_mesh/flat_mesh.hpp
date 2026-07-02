// Copyright 2026 Travis West
#pragma once
#include <memory>
#include <string_view>

#include "eyeballs_node_abi.hpp"
#include "render_payloads.hpp"

// flat_mesh: a shader-specific node for UNLIT, FLAT-shaded geometry — the
// `flat` GLSL qualifier takes each triangle's provoking-vertex color with no
// interpolation and no lighting. The distinct shading the lit nodes don't
// offer (cube_node/color_mesh = lit single color; vertex_color_mesh = lit
// smooth). Wraps raw geometry (MeshPtr) into Surface + Mesh for a draw node;
// good for faceted/low-poly looks and debug-colored meshes. GL lives only in
// render_region (ABI v8).
namespace flat_mesh_detail {
inline constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=2) in vec4 aColor;
layout(location=3) in vec3 aOffset;
uniform mat4 uMVP;
flat out vec4 vColor;
void main() { vColor = aColor; gl_Position = uMVP * vec4(aPos + aOffset, 1.0); }
)";
inline constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
flat in vec4 vColor;
uniform vec4 uTint;
out vec4 fragColor;
void main() { fragColor = vColor * uTint; }
)";
}  // namespace flat_mesh_detail

struct FlatMeshNode {
    static consteval std::string_view name() { return "flat_mesh"; }
    static consteval std::string_view source_header() {
        return "components/flat_mesh/flat_mesh.hpp";
    }
    struct endpoints {
        ::in<MeshPtr> geometry;
        ::in<Span> positions;  // N×3 instance offsets; unwired ⇒ one at origin
        normalled_in<float, fp(0.f), fp(1.f), fp(1.f)> tint_r;
        normalled_in<float, fp(0.f), fp(1.f), fp(1.f)> tint_g;
        normalled_in<float, fp(0.f), fp(1.f), fp(1.f)> tint_b;
        ::out<Surface> surface;
        ::out<Mesh> mesh;
    } endpoints;
    void operator()(double) {
        if (!shader_)
            shader_ = std::make_shared<ShaderData>(
                ShaderData{flat_mesh_detail::kVert, flat_mesh_detail::kFrag});
        Mesh m;
        m.geometry = endpoints.geometry.get();
        m.mode = Primitive::Triangles;
        Span pos = endpoints.positions.get();
        if (pos.data && pos.rows > 0 && pos.cols == 3)
            m.instances.push_back({"aOffset", pos});
        else
            m.instances.push_back({"aOffset", Span{origin_, 1, 3, Axis::Item, Axis::Cell}});
        endpoints.mesh.value = std::move(m);

        Surface s;
        s.shader = shader_;
        s.uniforms.push_back(
            {"uTint",
             Eigen::Vector4f(
                 endpoints.tint_r.get(), endpoints.tint_g.get(), endpoints.tint_b.get(), 1.0f)});
        endpoints.surface.value = std::move(s);
    }

private:
    Shader shader_;
    float origin_[3] = {0.0f, 0.0f, 0.0f};
};
