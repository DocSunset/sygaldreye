// Copyright 2025 Travis West
#include "vr_panel.hpp"
#include "rgba_shader.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Geometry>
#include <array>
#include <cmath>

std::optional<RayHit> VrPanel::intersect(const Eigen::Vector3f& origin,
                                          const Eigen::Vector3f& dir) const {
    float denom = normal.dot(dir);
    if (std::abs(denom) < 1e-6f) return std::nullopt;

    float t = normal.dot(position - origin) / denom;
    if (t < 0.0f) return std::nullopt;

    Eigen::Vector3f hit = origin + t * dir;
    Eigen::Vector3f local = hit - position;

    // Build a local frame: right = up x normal, up = arbitrary non-parallel
    Eigen::Vector3f up_ref = (std::abs(normal.y()) < 0.9f)
        ? Eigen::Vector3f{0, 1, 0} : Eigen::Vector3f{1, 0, 0};
    Eigen::Vector3f right = up_ref.cross(normal).normalized();
    Eigen::Vector3f up    = normal.cross(right).normalized();

    float u = local.dot(right);
    float v = local.dot(up);

    if (std::abs(u) > width * 0.5f || std::abs(v) > height * 0.5f)
        return std::nullopt;

    RayHit h;
    h.t  = t;
    h.uv = {(u / width) + 0.5f, (v / height) + 0.5f};
    return h;
}

void VrPanel::draw(const Eigen::Matrix4f& vp, RgbaShader& shader) const {
    // Build model matrix: translate to position, orient normal to +Z
    Eigen::Quaternionf q = Eigen::Quaternionf::FromTwoVectors(
        Eigen::Vector3f{0, 0, 1}, normal);
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    model.block<3,3>(0,0) = q.toRotationMatrix() *
        Eigen::Scaling(width * 0.5f, height * 0.5f, 1.0f).toDenseMatrix().block<3,3>(0,0);
    model.block<3,1>(0,3) = position;

    // Unit quad in [-1,1]x[-1,1]
    static const std::array<float,12> kQuad = {
        -1,-1, 0,   1,-1, 0,   1, 1, 0,  -1, 1, 0
    };
    static const std::array<unsigned short,6> kIdx = {0,1,2, 0,2,3};

    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(kQuad.size() * sizeof(float)),
                 kQuad.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(kIdx.size() * sizeof(unsigned short)),
                 kIdx.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    shader.use();
    shader.set_mvp(vp * model);
    shader.set_color(color);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

    glBindVertexArray(0);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}
