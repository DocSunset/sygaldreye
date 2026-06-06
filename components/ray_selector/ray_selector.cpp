// Copyright 2025 Travis West
#include "ray_selector.hpp"
#include <Eigen/Geometry>
#include <limits>

namespace {
Eigen::Vector3f xr_to_eigen(const XrVector3f& v) {
    return {v.x, v.y, v.z};
}

Eigen::Quaternionf xr_to_eigen(const XrQuaternionf& q) {
    return {q.w, q.x, q.y, q.z};  // Eigen: (w,x,y,z)
}
}

std::optional<PanelHit> RaySelector::test(const XrPosef& pose,
                                           const std::vector<VrPanel>& panels) {
    Eigen::Vector3f origin = xr_to_eigen(pose.position);
    Eigen::Quaternionf rot = xr_to_eigen(pose.orientation);
    Eigen::Vector3f dir    = rot * Eigen::Vector3f{0, 0, -1};

    float best_t = std::numeric_limits<float>::infinity();
    std::optional<PanelHit> result;

    for (int i = 0; i < static_cast<int>(panels.size()); ++i) {
        auto h = panels[static_cast<size_t>(i)].intersect(origin, dir);
        if (h && h->t < best_t) {
            best_t = h->t;
            result = PanelHit{i, *h};
        }
    }
    return result;
}

void RaySelector::draw_ray(const XrPosef& /*pose*/, float /*length*/,
                            const Eigen::Matrix4f& /*vp*/) {
    // Intentionally empty stub — visual ray debug not required.
}
