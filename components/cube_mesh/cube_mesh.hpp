#pragma once
#include <Eigen/Core>
#include "cube_shader.hpp"
#include "cube_geometry.hpp"

struct CubeMesh {
    void init();
    void begin_batch() const;
    static void end_batch();
    void draw(const Eigen::Matrix4f& mvp) const;

private:
    CubeShader shader_;
    CubeGeometry geometry_;
};
