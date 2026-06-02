#include "scene.hpp"
#include <Eigen/Geometry>

void Scene::update(double time) {
    float t = static_cast<float>(time);
    float ax = 0.4f * sinf(t * 0.19f + 0.0f);
    float ay = 0.4f * sinf(t * 0.27f + 1.1f);
    float az = 0.4f * sinf(t * 0.13f + 2.3f);
    Eigen::Quaternionf q =
        Eigen::AngleAxisf(ax, Eigen::Vector3f::UnitX()) *
        Eigen::AngleAxisf(ay, Eigen::Vector3f::UnitY()) *
        Eigen::AngleAxisf(az, Eigen::Vector3f::UnitZ());
    Eigen::Affine3f xf =
        Eigen::Translation3f(0.0f, 1.5f, -5.0f) *
        q *
        Eigen::Scaling(0.4f);
    cubes_.resize(1);
    cubes_[0].model = xf.matrix();
}

std::span<const CubeInstance> Scene::cubes() const {
    return std::span<const CubeInstance>(cubes_);
}

void Scene::set_controller_poses(const Eigen::Matrix4f* left_model, bool left_valid,
                                 const Eigen::Matrix4f* right_model, bool right_valid) {
    cubes_.resize(1);
    if (left_valid  && left_model)  cubes_.push_back({*left_model});
    if (right_valid && right_model) cubes_.push_back({*right_model});
}
