#include "cube_mesh.hpp"

void CubeMesh::init() {
    shader_.init();
    geometry_.init();
}

void CubeMesh::begin_batch() const {
    shader_.use();
    geometry_.bind();
}

void CubeMesh::end_batch() {
    CubeGeometry::unbind();
}

void CubeMesh::draw(const Eigen::Matrix4f& mvp) const {
    shader_.set_mvp(mvp);
    CubeGeometry::draw_elements();
}
