#include "scene.hpp"
#include <Eigen/Geometry>

namespace {
constexpr float kRotationAmplitude = 0.4F;
constexpr float kRotationSpeedX = 0.19F;
constexpr float kRotationSpeedY = 0.27F;
constexpr float kRotationSpeedZ = 0.13F;
constexpr float kRotationPhaseX = 0.0F;
constexpr float kRotationPhaseY = 1.1F;
constexpr float kRotationPhaseZ = 2.3F;
constexpr float kCubePositionX = 0.0F;
constexpr float kCubePositionY = 1.5F;
constexpr float kCubePositionZ = -5.0F;
constexpr float kCubeScale = 0.4F;
}

void Scene::update(double time) {
    float t = static_cast<float>(time);
    float ax = kRotationAmplitude * sinf(t * kRotationSpeedX + kRotationPhaseX);
    float ay = kRotationAmplitude * sinf(t * kRotationSpeedY + kRotationPhaseY);
    float az = kRotationAmplitude * sinf(t * kRotationSpeedZ + kRotationPhaseZ);
    Eigen::Quaternionf q =
        Eigen::AngleAxisf(ax, Eigen::Vector3f::UnitX()) *
        Eigen::AngleAxisf(ay, Eigen::Vector3f::UnitY()) *
        Eigen::AngleAxisf(az, Eigen::Vector3f::UnitZ());
    Eigen::Affine3f xf =
        Eigen::Translation3f(kCubePositionX, kCubePositionY, kCubePositionZ) *
        q *
        Eigen::Scaling(kCubeScale);
    world_cube_.model = xf.matrix();
}

std::span<const CubeInstance> Scene::cubes() const {
    cubes_cache_.clear();
    cubes_cache_.push_back(world_cube_);
    for (const auto& c : controller_cubes_) {
        if (c) { cubes_cache_.push_back(*c); }
    }
    return std::span<const CubeInstance>(cubes_cache_);
}

void Scene::set_controller_poses(std::optional<Eigen::Matrix4f> left_model,
                                 std::optional<Eigen::Matrix4f> right_model) {
    controller_cubes_.at(0U) = left_model  ? std::optional<CubeInstance>({*left_model})  : std::nullopt;
    controller_cubes_.at(1U) = right_model ? std::optional<CubeInstance>({*right_model}) : std::nullopt;
}
