#include "scene.hpp"
#include "vr_math.hpp"
#include <Eigen/Geometry>

namespace {
constexpr float kHandCubeOffsetXM = 0.0F;
constexpr float kHandCubeOffsetYM = 0.0F;
constexpr float kHandCubeOffsetZM = 0.0F;
constexpr float kHandCubeScale    = 0.035F;
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

Scene::Scene() {
    world_cube_.material.diffuse  = {0.2F, 0.4F, 0.9F};
    world_cube_.material.ambient  = {0.02F, 0.04F, 0.09F};
    world_cube_.material.specular = {0.4F, 0.4F, 0.5F};
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
    cube_count_ = 0;
    cubes_cache_.at(cube_count_++) = world_cube_;
    for (size_t i = 0; i < controller_cubes_.size(); ++i) {
        if (controller_cubes_.at(i)) {
            cubes_cache_.at(cube_count_++) = *controller_cubes_.at(i);
        }
    }
    return std::span<const CubeInstance>(cubes_cache_.data(), cube_count_);
}

static Eigen::Matrix4f hand_cube_model(const XrPosef& pose) {
    Eigen::Matrix4f scale_m = Eigen::Matrix4f::Identity();
    scale_m(0,0) = scale_m(1,1) = scale_m(2,2) = kHandCubeScale;
    Eigen::Matrix4f local_T = Eigen::Matrix4f::Identity();
    local_T(0,3) = kHandCubeOffsetXM;
    local_T(1,3) = kHandCubeOffsetYM;
    local_T(2,3) = kHandCubeOffsetZM;
    return (pose_to_world(pose) * local_T * scale_m).eval();
}

void Scene::update_labels(const Eigen::Matrix4f& right_cube_model) {
    struct FaceDef { const char* text; Eigen::Vector3f normal; Eigen::Vector3f up; };
    // Grip pose axes: +Y toward user, -Z world-up, -Y forward (tip direction), +Z down
    static const FaceDef faces[] = {
        {"toward user", { 0.0F,  1.0F,  0.0F}, { 0.0F,  0.0F, -1.0F}},
        {"forward",     { 0.0F, -1.0F,  0.0F}, { 0.0F,  0.0F, -1.0F}},
        {"top",         { 0.0F,  0.0F, -1.0F}, { 0.0F, -1.0F,  0.0F}},
        {"bottom",      { 0.0F,  0.0F,  1.0F}, { 0.0F,  1.0F,  0.0F}},
    };
    constexpr float kTextOffset = 0.038F;
    constexpr float kTextScale  = 0.005F;
    labels_cache_.clear();
    Eigen::Vector3f cube_center = right_cube_model.block<3,1>(0,3);
    Eigen::Matrix3f rot3        = right_cube_model.block<3,3>(0,0);
    for (const auto& face : faces) {
        Eigen::Vector3f world_normal = (rot3 * face.normal).normalized();
        Eigen::Vector3f world_up     = (rot3 * face.up).normalized();
        Eigen::Vector3f world_pos    = cube_center + world_normal * kTextOffset;
        Eigen::Vector3f text_forward = -world_normal;
        // text_forward × world_up gives the viewer's right when looking at the face head-on
        Eigen::Vector3f text_right   = text_forward.cross(world_up).normalized();
        Eigen::Vector3f text_up      = world_normal.cross(text_right).normalized();

        Eigen::Matrix4f mat = Eigen::Matrix4f::Identity();
        mat.block<3,1>(0,0) = text_right;
        mat.block<3,1>(0,1) = text_up;
        mat.block<3,1>(0,2) = text_forward;
        mat.block<3,1>(0,3) = world_pos;

        Eigen::Matrix4f scale = Eigen::Matrix4f::Identity();
        scale(0,0) = scale(1,1) = scale(2,2) = kTextScale;

        labels_cache_.push_back({face.text, mat * scale});
    }
}

std::span<const Label> Scene::labels() const {
    return {labels_cache_.data(), labels_cache_.size()};
}

static CubeInstance hand_cube_instance(const XrPosef& pose, const Material& mat) {
    CubeInstance inst;
    inst.model    = hand_cube_model(pose);
    inst.material = mat;
    return inst;
}

void Scene::set_controller_poses(std::optional<XrPosef> left_pose,
                                 std::optional<XrPosef> right_pose) {
    static const Material kLeftMat  = [] {
        Material m; m.diffuse={0.9F,0.2F,0.15F}; m.ambient={0.09F,0.02F,0.015F}; return m;
    }();
    static const Material kRightMat = [] {
        Material m; m.diffuse={0.15F,0.8F,0.25F}; m.ambient={0.015F,0.08F,0.025F}; return m;
    }();
    controller_cubes_.at(0U) = left_pose
        ? std::optional<CubeInstance>(hand_cube_instance(*left_pose, kLeftMat))
        : std::nullopt;
    if (right_pose) {
        Eigen::Matrix4f right_model = hand_cube_model(*right_pose);
        CubeInstance inst;
        inst.model    = right_model;
        inst.material = kRightMat;
        controller_cubes_.at(1U) = inst;
        update_labels(right_model);
    } else {
        controller_cubes_.at(1U) = std::nullopt;
        labels_cache_.clear();
    }
}
