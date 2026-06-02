#include "scene.hpp"
#include <Eigen/Geometry>

void Scene::update(double time) {
    float angle = static_cast<float>(time) * 1.0f;
    Eigen::Affine3f xf =
        Eigen::Translation3f(0.0f, 1.5f, -2.0f) *
        Eigen::AngleAxisf(angle, Eigen::Vector3f::UnitY());
    cubes_.resize(1);
    cubes_[0].model = xf.matrix();
}

std::span<const CubeInstance> Scene::cubes() const {
    return std::span<const CubeInstance>(cubes_);
}

void Scene::set_controller_poses() {}
