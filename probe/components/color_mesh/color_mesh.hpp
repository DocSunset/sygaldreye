// Copyright 2026 Travis West
#pragma once
#include <memory>
#include <string_view>

#include <Eigen/Core>

#include "eyeballs_node_abi.hpp"
#include "render_payloads.hpp"

// color_mesh: a shader-specific node. Wraps raw geometry (MeshPtr) and a color
// into a Surface (a simple lit, single-color program) + a Mesh (drawable), the
// two payloads the draw node consumes. The first such node — proves the
// render-as-nodes pipeline end to end. Uniform set + attribute layout are
// fixed by THIS shader, so the bundles are correct by construction.
namespace color_mesh_detail {
inline constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec4 aColor;
layout(location=3) in vec3 aOffset;
uniform mat4 uMVP;
out vec3 vNormal;
void main() {
    vNormal = aNormal;
    gl_Position = uMVP * vec4(aPos + aOffset, 1.0);
}
)";
inline constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec3 vNormal;
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    float d = max(dot(normalize(vNormal), normalize(vec3(0.3, 0.8, 0.5))), 0.0);
    fragColor = vec4(uColor.rgb * (0.3 + 0.7 * d), uColor.a);
}
)";
}  // namespace color_mesh_detail

struct ColorMeshNode {
    static consteval std::string_view name() { return "color_mesh"; }
    static consteval std::string_view source_header() {
        return "components/color_mesh/color_mesh.hpp";
    }
    struct endpoints {
        ::in<MeshPtr>         geometry;
        ::in<Span>            positions;  // N×3 instance offsets; unwired ⇒ one at origin
        ::in<Eigen::Vector4f> color;      // optional override of the r/g/b params
        normalled_in<float, fp(0.f), fp(1.f), fp(0.8f)> r;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.8f)> g;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.8f)> b;
        ::out<Surface>        surface;
        ::out<Mesh>           mesh;
    } endpoints;
    void operator()(double) {
        if (!shader_)
            shader_ = std::make_shared<ShaderData>(
                ShaderData{color_mesh_detail::kVert, color_mesh_detail::kFrag});
        Mesh m;
        m.geometry = endpoints.geometry.get();
        m.mode     = Primitive::Triangles;
        // Rank polymorphism: an N×3 positions Span ⇒ N instances; unwired ⇒
        // one instance at the origin. Same node draws a cube or a forest.
        Span pos = endpoints.positions.get();
        if (pos.data && pos.rows > 0 && pos.cols == 3)
            m.instances.push_back({"aOffset", pos});
        else
            m.instances.push_back({"aOffset", Span{origin_, 1, 3, Axis::Item, Axis::Cell}});
        endpoints.mesh.value = std::move(m);

        Eigen::Vector4f c =
            endpoints.color.src
                ? *endpoints.color.src
                : Eigen::Vector4f(endpoints.r.get(), endpoints.g.get(), endpoints.b.get(), 1.0f);
        Surface s;
        s.shader = shader_;
        s.uniforms.push_back({"uColor", c});
        endpoints.surface.value = std::move(s);
    }

   private:
    Shader shader_;
    float  origin_[3] = {0.0f, 0.0f, 0.0f};
};
