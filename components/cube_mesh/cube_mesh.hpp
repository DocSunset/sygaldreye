#pragma once
#include <Eigen/Core>
#include "gl_program.hpp"

struct CubeMesh {
    void init();
    void draw(const Eigen::Matrix4f& mvp);
private:
    unsigned vao_ = 0, vbo_ = 0;
    GlProgram prog_;
};
