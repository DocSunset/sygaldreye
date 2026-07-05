// Copyright 2026 Travis West
#pragma once
#include <memory>
#include <string_view>

#include <Eigen/Core>

#include "eyeballs_node_abi.hpp"
#include "render_payloads.hpp"
#include "tri_mesh.hpp"

// sprite: a shader-specific node for camera-facing billboards. Owns a unit
// quad and a facing shader that builds each corner from the render-time
// uCameraRight/uCameraUp (injected by render_region). Fed an N×3 positions
// Span it draws N soft round sprites in one instanced, blended draw — the
// particle/billboard case, expressed as graph nodes with no GL in the node.
namespace sprite_detail {
inline constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;      // quad corner in [-0.5,0.5]^2
layout(location=3) in vec3 aOffset;   // per-instance world position
uniform mat4 uMVP;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform float uSize;
out vec2 vUV;
void main() {
    vUV = aPos.xy + 0.5;
    vec3 world = aOffset + (aPos.x * uSize) * uCameraRight + (aPos.y * uSize) * uCameraUp;
    gl_Position = uMVP * vec4(world, 1.0);
}
)";
inline constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec2 vUV;
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    float d = length(vUV - 0.5) * 2.0;
    float a = smoothstep(1.0, 0.0, d);
    fragColor = vec4(uColor.rgb, uColor.a * a);
}
)";
}  // namespace sprite_detail

struct SpriteNode {
    static consteval std::string_view name() { return "sprite"; }
    static consteval std::string_view source_header() {
        return "components/sprite/sprite.hpp";
    }
    struct endpoints {
        ::in<Span>                                       positions;  // N×3; unwired ⇒ one
        normalled_in<float, fp(0.01f), fp(10.f), fp(0.3f)> size;
        normalled_in<float, fp(0.f), fp(1.f), fp(1.0f)>    r;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.8f)>    g;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.4f)>    b;
        ::out<Surface>                                   surface;
        ::out<Mesh>                                      mesh;
    } endpoints;
    void operator()(double) {
        if (!quad_) quad_ = make_quad();
        if (!shader_)
            shader_ = std::make_shared<ShaderData>(
                ShaderData{sprite_detail::kVert, sprite_detail::kFrag});
        Mesh m;
        m.geometry = quad_;
        m.mode     = Primitive::Triangles;
        Span pos   = endpoints.positions.get();
        if (pos.data && pos.rows > 0 && pos.cols == 3)
            m.instances.push_back({"aOffset", pos});
        else
            m.instances.push_back({"aOffset", Span{origin_, 1, 3, Axis::Item, Axis::Cell}});
        endpoints.mesh.value = std::move(m);

        Surface s;
        s.shader = shader_;
        s.uniforms.push_back({"uSize", endpoints.size.get()});
        s.uniforms.push_back(
            {"uColor", Eigen::Vector4f(endpoints.r.get(), endpoints.g.get(), endpoints.b.get(), 1.0f)});
        s.blend       = true;
        s.depth_write = false;  // blended sprites test depth but don't occlude
        s.cull_back   = false;
        endpoints.surface.value = std::move(s);
    }

   private:
    static MeshPtr make_quad() {
        auto m = std::make_shared<TriMeshData>();
        auto v = [](float x, float y) {
            TriVertex t;
            t.position = Eigen::Vector3f(x, y, 0.0f);
            t.normal   = Eigen::Vector3f(0.0f, 0.0f, 1.0f);
            t.color    = Eigen::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
            return t;
        };
        m->vertices = {v(-0.5f, -0.5f), v(0.5f, -0.5f), v(0.5f, 0.5f), v(-0.5f, 0.5f)};
        m->indices  = {0, 1, 2, 0, 2, 3};
        return m;
    }
    MeshPtr quad_;
    Shader  shader_;
    float   origin_[3] = {0.0f, 0.0f, 0.0f};
};
