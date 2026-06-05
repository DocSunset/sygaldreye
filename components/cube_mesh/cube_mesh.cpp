#include "cube_mesh.hpp"
#include "light.hpp"
#include "material.hpp"

void CubeMesh::init() {
    shader_.init();
    geometry_.init();
}

void CubeMesh::begin_batch(std::span<const Light> lights, const Eigen::Vector3f& view_pos) const {
    shader_.use();
    shader_.set_lights(lights);
    shader_.set_view_pos(view_pos);
    geometry_.bind();
}

void CubeMesh::end_batch() {
    CubeGeometry::unbind();
}

void CubeMesh::draw(const Eigen::Matrix4f& mvp, const Eigen::Matrix4f& model, const Material& mat) const {
    shader_.set_mvp(mvp);
    shader_.set_model(model);
    shader_.set_material(mat);
    CubeGeometry::draw_elements();
}
