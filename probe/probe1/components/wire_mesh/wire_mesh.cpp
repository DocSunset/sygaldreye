// Copyright 2026 Travis West
#include "wire_mesh.hpp"

#include <memory>

#include <Eigen/Core>

#include "tri_mesh.hpp"

namespace {
constexpr int kBezierSegs = 14;

// Unlit per-vertex color, drawn as GL_LINES. uMVP injected by render_region.
constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
out vec4 vColor;
void main() {
    vColor = aColor;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() { fragColor = vColor; }
)";

TriVertex line_vertex(const Eigen::Vector3f& p, const float* c) {
    TriVertex v;
    v.position = p;
    v.normal   = Eigen::Vector3f::Zero();
    v.color    = Eigen::Vector4f(c[0], c[1], c[2], c[3]);
    return v;
}
}  // namespace

void WireMeshNode::operator()(double) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});

    // Tessellate the span into GL_LINES vertex pairs (rebuilt each frame).
    auto data = std::make_shared<TriMeshData>();
    Span s    = endpoints.wires.get();
    if (s.data && s.cols == 10) {
        for (int r = 0; r < s.rows; ++r) {
            const float*    row = s.data + std::size_t(r) * 10;
            Eigen::Vector3f p0{row[0], row[1], row[2]};
            Eigen::Vector3f p3{row[3], row[4], row[5]};
            const float*    c  = row + 6;
            // Control points lean toward the viewer (cards face +Z).
            Eigen::Vector3f p1   = p0 + Eigen::Vector3f{0, 0, -0.1f};
            Eigen::Vector3f p2   = p3 + Eigen::Vector3f{0, 0, -0.1f};
            Eigen::Vector3f prev = p0;
            for (int i = 1; i <= kBezierSegs; ++i) {
                float           t = float(i) / float(kBezierSegs);
                float           t2 = t * t, t3 = t2 * t;
                float           u = 1.f - t, u2 = u * u, u3 = u2 * u;
                Eigen::Vector3f p = u3 * p0 + 3.f * u2 * t * p1 + 3.f * u * t2 * p2 + t3 * p3;
                data->vertices.push_back(line_vertex(prev, c));  // GL_LINES pair
                data->vertices.push_back(line_vertex(p, c));
                prev = p;
            }
        }
    }

    Mesh m;
    m.geometry = std::move(data);  // no indices → draw_arrays(GL_LINES)
    m.mode     = Primitive::Lines;
    m.dynamic  = true;
    endpoints.mesh.value = std::move(m);

    Surface surf;
    surf.shader    = shader_;
    surf.cull_back = false;  // lines
    endpoints.surface.value = std::move(surf);
}
