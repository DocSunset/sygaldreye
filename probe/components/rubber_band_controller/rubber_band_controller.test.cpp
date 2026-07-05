// Copyright 2025 Travis West
#include "rubber_band_controller.hpp"

#include <gtest/gtest.h>

// Grab/offset behaviour + emitted payload. No GL — geometry is a CPU mesh.

struct Rig {
    RubberBandController r;
    Eigen::Vector3f lpos{9.f, 9.f, 9.f}, rpos{9.f, 9.f, 9.f};
    Rig() {
        r.endpoints.anchor_x.fallback = 0.f;
        r.endpoints.anchor_y.fallback = 1.f;
        r.endpoints.anchor_z.fallback = -0.5f;
        r.endpoints.left_pos.src = &lpos;
        r.endpoints.right_pos.src = &rpos;
    }
};

TEST(RubberBand, DefaultOffsetIsUpwardTenCm) {
    Rig r;
    r.r(0.0);
    EXPECT_TRUE(r.r.endpoints.offset.value.isApprox(Eigen::Vector3f(0.f, 0.1f, 0.f), 1e-5f));
}

TEST(RubberBand, GrabbingControlMovesOnlyControl) {
    Rig r;
    r.r(0.0);                     // seed at anchor (0,1,-0.5), control (0,1.1,-0.5)
    r.lpos = {0.f, 1.1f, -0.5f};  // on the control sphere
    r.r.endpoints.left_grip.fallback = 1.f;
    r.r(0.0);                      // acquire
    r.lpos = {0.3f, 1.1f, -0.5f};  // drag control +x
    r.r(0.0);
    // control moved, anchor stayed → offset grew in +x
    EXPECT_GT(r.r.endpoints.offset.value.x(), 0.2f);
}

TEST(RubberBand, EmitsNonEmptyMeshAndSurface) {
    Rig r;
    r.r(0.0);
    auto& m = r.r.endpoints.mesh.value;
    ASSERT_TRUE(m.geometry);
    EXPECT_GT(m.geometry->vertices.size(), 0u);
    EXPECT_GT(m.geometry->indices.size(), 0u);
    EXPECT_TRUE(r.r.endpoints.surface.value.shader);
}
