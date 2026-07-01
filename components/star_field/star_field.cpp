// Copyright 2025 Travis West
#include "star_field.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

#include "tri_mesh.hpp"

namespace {

// Stars generated from gl_VertexID; vertex positions ignored. Finite radius +
// normal uMVP so they depth-sort behind closer geometry (order-independent).
// uMVP injected by render_region.
constexpr const char* kVert = R"(#version 300 es
precision mediump float;
uniform mat4  uMVP;
uniform float uStarAlpha;
uniform float uRadius;
uint uhash(uint x) {
    x = ((x >> 16u) ^ x) * 0x45d9f3bu;
    x = ((x >> 16u) ^ x) * 0x45d9f3bu;
    return (x >> 16u) ^ x;
}
float fhash(uint x) { return float(uhash(x)) * (1.0 / 4294967296.0); }
void main() {
    uint  id    = uint(gl_VertexID);
    float theta = 6.28318530 * fhash(id);
    float u     = fhash(id + 2000u) * 2.0 - 1.0;
    float r     = sqrt(max(0.0, 1.0 - u * u));
    vec3  pos   = uRadius * vec3(cos(theta) * r, abs(u), sin(theta) * r);
    gl_Position  = uMVP * vec4(pos, 1.0);
    gl_PointSize = 1.0 + fhash(id + 4000u) * 2.0;
}
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
uniform float uStarAlpha;
out vec4 fragColor;
void main() {
    vec2  uv = gl_PointCoord * 2.0 - 1.0;
    float d  = dot(uv, uv);
    float a  = smoothstep(1.0, 0.3, d) * uStarAlpha;
    fragColor = vec4(1.0, 1.0, 1.0, a);
}
)";

MeshPtr make_points(int n) {
    auto m = std::make_shared<TriMeshData>();
    m->vertices.resize(static_cast<size_t>(std::max(0, n)));  // positions unused
    return m;
}

}  // namespace

void StarField::operator()(double) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});
    int n = std::max(0, static_cast<int>(endpoints.star_count.get()));
    if (n != count_) {
        points_ = make_points(n);
        count_  = n;
    }

    const float alpha = std::clamp(-endpoints.sun_elevation.get() * 3.0f, 0.0f, 1.0f);

    Mesh m;
    m.geometry = points_;
    m.mode     = Primitive::Points;
    endpoints.mesh.value = std::move(m);

    Surface s;
    s.shader      = shader_;
    s.blend       = true;
    s.depth_write = false;  // background points don't occlude
    s.cull_back   = false;
    s.uniforms.push_back({"uStarAlpha", alpha});
    s.uniforms.push_back({"uRadius", endpoints.radius.get()});
    endpoints.surface.value = std::move(s);
}
