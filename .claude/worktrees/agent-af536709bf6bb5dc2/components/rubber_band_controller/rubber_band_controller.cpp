// Copyright 2025 Travis West
#include "rubber_band_controller.hpp"

#include <Eigen/Geometry>
#include <string>

#include "gl_program.hpp"
#include "grab_detector.hpp"
#include "sphere_geometry.hpp"

namespace {

constexpr float kSphereRadius  = 0.025F;
constexpr float kBandRadius    = 0.005F;
constexpr float kDefaultOffset = 0.1F;
constexpr float kLabelScale    = 0.004F;
constexpr int   kSphereSlices  = 16;
constexpr int   kSphereLat     = 8;
constexpr int   kCylSlices     = 12;

Eigen::Matrix4f sphere_model(Eigen::Vector3f pos) // NOLINT(misc-use-internal-linkage)
{
    return (Eigen::Translation3f(pos) * Eigen::Scaling(kSphereRadius)).matrix();
}

} // namespace

RubberBandController RubberBandController::create(
    Eigen::Vector3f anchor_pos,
    std::function<std::string(Eigen::Vector3f)> label_fn)
{
    RubberBandController rbc{};

    rbc.sphere_mesh_.emplace(SphereMesh::create(make_sphere(kSphereSlices, kSphereLat)));
    rbc.cylinder_mesh_.emplace(CylinderMesh::create(kCylSlices));
    rbc.rgba_shader_.create();
    rbc.text_mesh_.init();

    rbc.targets_[0].position = anchor_pos;
    rbc.targets_[0].radius   = kSphereRadius;
    rbc.targets_[1].position = anchor_pos + Eigen::Vector3f{0.0F, kDefaultOffset, 0.0F};
    rbc.targets_[1].radius   = kSphereRadius;

    rbc.label_fn_ = label_fn ? std::move(label_fn)
                              : [](Eigen::Vector3f off) { return std::to_string(off.norm()); };

    return rbc;
}

void RubberBandController::update(std::span<const HandState> hands)
{
    const Eigen::Vector3f prev_anchor = targets_[0].position;
    update_grabs(hands, std::span<GrabTarget>(targets_));

    if (targets_[0].grabbed && targets_[0].grabbing_hand >= 0 &&
        targets_[0].grabbing_hand < static_cast<int>(hands.size())) {
        const auto& hand = hands[static_cast<size_t>(targets_[0].grabbing_hand)];
        targets_[0].position = hand.position + targets_[0].grab_offset;
        if (!targets_[1].grabbed) {
            targets_[1].position += targets_[0].position - prev_anchor;
        }
    }
    if (targets_[1].grabbed && targets_[1].grabbing_hand >= 0 &&
        targets_[1].grabbing_hand < static_cast<int>(hands.size())) {
        const auto& hand = hands[static_cast<size_t>(targets_[1].grabbing_hand)];
        targets_[1].position = hand.position + targets_[1].grab_offset;
    }
}

void RubberBandController::draw(const Eigen::Matrix4f& view_proj) const
{
    const Eigen::Vector3f& anchor  = targets_[0].position;
    const Eigen::Vector3f& control = targets_[1].position;

    rgba_shader_.use();

    rgba_shader_.set_mvp(view_proj * sphere_model(anchor));
    rgba_shader_.set_color({1.0F, 0.8F, 0.0F, 0.8F});
    sphere_mesh_->draw();

    rgba_shader_.set_mvp(view_proj * sphere_model(control));
    rgba_shader_.set_color({0.2F, 0.6F, 1.0F, 1.0F});
    sphere_mesh_->draw();

    rgba_shader_.set_mvp(view_proj * cylinder_transform(anchor, control, kBandRadius));
    rgba_shader_.set_color({0.9F, 0.2F, 0.2F, 1.0F});
    cylinder_mesh_->draw();

    const Eigen::Matrix4f label_mvp =
        view_proj *
        (Eigen::Translation3f(anchor + Eigen::Vector3f{0.0F, -(kSphereRadius * 2.0F), 0.0F}) *
         Eigen::Scaling(kLabelScale))
            .matrix();
    text_mesh_.draw(label_fn_(offset()), label_mvp);
}

Eigen::Vector3f RubberBandController::offset() const
{
    return targets_[1].position - targets_[0].position;
}

RubberBandController::~RubberBandController() = default;
