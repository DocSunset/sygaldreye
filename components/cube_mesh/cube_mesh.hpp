#pragma once
#include <utility>
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include "gl_program.hpp"

struct CubeMesh {
    CubeMesh() = default;
    ~CubeMesh() {
        if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
        if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
        if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
    }

    CubeMesh(const CubeMesh&) = delete;
    CubeMesh& operator=(const CubeMesh&) = delete;

    CubeMesh(CubeMesh&& src) noexcept
        : vao_(src.vao_), vbo_(src.vbo_), ebo_(src.ebo_),
          prog_(std::move(src.prog_)) {
        src.vao_ = 0U;
        src.vbo_ = 0U;
        src.ebo_ = 0U;
    }
    CubeMesh& operator=(CubeMesh&& src) noexcept {
        if (this != &src) {
            if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
            if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
            if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
            vao_ = src.vao_; src.vao_ = 0U;
            vbo_ = src.vbo_; src.vbo_ = 0U;
            ebo_ = src.ebo_; src.ebo_ = 0U;
            prog_ = std::move(src.prog_);
        }
        return *this;
    }

    void init();
    void draw(const Eigen::Matrix4f& mvp);
private:
    GLuint vao_ = 0U;
    GLuint vbo_ = 0U;
    GLuint ebo_ = 0U;
    GlProgram prog_ = {};
};
