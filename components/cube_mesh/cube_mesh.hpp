#pragma once
#include <Eigen/Core>
#include <span>
#include "lit_shader.hpp"
#include "cube_geometry.hpp"

struct Light;
struct Material;

struct CubeMesh {
    void init();
    void begin_batch(std::span<const Light> lights, const Eigen::Vector3f& view_pos) const;
    static void end_batch();
    void draw(const Eigen::Matrix4f& mvp, const Eigen::Matrix4f& model, const Material& mat) const;

private:
    LitShader shader_;
    CubeGeometry geometry_;
};
