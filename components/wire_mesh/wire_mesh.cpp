// Copyright 2026 Travis West
#include "wire_mesh.hpp"

#include <GLES3/gl3.h>

#include <cstdio>
#include <Eigen/Core>

namespace {
constexpr int kBezierSegs = 14;

constexpr const char* kVert = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;
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
}  // namespace

void WireMeshNode::operator()(double) {
    // Tessellate at tick so the draw only streams vertices. Copy out of the
    // producer's span: the draw runs after the tick.
    Span s = endpoints.wires.get();
    verts_.clear();
    if (s.data && s.cols == 10) {
        for (int r = 0; r < s.rows; ++r) {
            const float* row = s.data + std::size_t(r) * 10;
            Eigen::Vector3f p0{row[0], row[1], row[2]};
            Eigen::Vector3f p3{row[3], row[4], row[5]};
            const float* c = row + 6;
            // Control points lean toward the viewer (cards face +Z).
            Eigen::Vector3f p1 = p0 + Eigen::Vector3f{0, 0, -0.1f};
            Eigen::Vector3f p2 = p3 + Eigen::Vector3f{0, 0, -0.1f};
            Eigen::Vector3f prev = p0;
            for (int i = 1; i <= kBezierSegs; ++i) {
                float t = float(i) / float(kBezierSegs);
                float t2 = t * t, t3 = t2 * t;
                float u = 1.f - t, u2 = u * u, u3 = u2 * u;
                Eigen::Vector3f p = u3 * p0 + 3.f * u2 * t * p1 + 3.f * u * t2 * p2 + t3 * p3;
                verts_.insert(
                    verts_.end(),
                    {prev.x(),
                     prev.y(),
                     prev.z(),
                     c[0],
                     c[1],
                     c[2],
                     c[3],
                     p.x(),
                     p.y(),
                     p.z(),
                     c[0],
                     c[1],
                     c[2],
                     c[3]});
                prev = p;
            }
        }
    }

    endpoints.render.value = [this](const Eigen::Matrix4f& pv) {
        if (verts_.empty()) return;
        if (!prog_) {
            auto p = GlProgram::build(kVert, kFrag);
            if (!p) {
                std::fprintf(stderr, "wire_mesh: shader failed\n");
                return;
            }
            prog_ = std::make_unique<GlProgram>(std::move(*p));
            glGenVertexArrays(1, &vao_);
            glGenBuffers(1, &vbo_);
        }
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(
            GL_ARRAY_BUFFER,
            GLsizeiptr(verts_.size() * sizeof(float)),
            verts_.data(),
            GL_STREAM_DRAW);
        constexpr GLsizei stride = 7 * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(3 * sizeof(float)));
        prog_->use();
        GlProgram::uniform(prog_->uniform_location("uMVP"), pv);
        glDrawArrays(GL_LINES, 0, GLsizei(verts_.size() / 7));
        glBindVertexArray(0);
    };
}
