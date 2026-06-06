#pragma once
#include "vr_panel.hpp"
#include <openxr/openxr.h>
#include <Eigen/Core>
#include <optional>
#include <vector>

struct PanelHit {
    int    panel_index;
    RayHit hit;
};

struct RaySelector {
    // Test ray from controller pose against panels; returns nearest hit.
    static std::optional<PanelHit> test(const XrPosef& pose,
                                         const std::vector<VrPanel>& panels);

    // Draw a visible ray line from the controller tip (stub).
    static void draw_ray(const XrPosef& pose, float length,
                         const Eigen::Matrix4f& vp);
};
