#include "vr_math.hpp"
#include <gtest/gtest.h>
#include <cmath>

static constexpr float kTol = 1e-5f;

// Identity pose: position=(0,0,0), orientation=(0,0,0,1)
static XrPosef identity_pose() {
    return XrPosef{
        .orientation = {0.f, 0.f, 0.f, 1.f},
        .position    = {0.f, 0.f, 0.f},
    };
}

TEST(VrMath, ViewIdentityPose) {
    auto m = view(identity_pose());
    EXPECT_TRUE(m.isApprox(Eigen::Matrix4f::Identity(), kTol));
}

TEST(VrMath, PoseToWorldIdentityPose) {
    auto m = pose_to_world(identity_pose());
    EXPECT_TRUE(m.isApprox(Eigen::Matrix4f::Identity(), kTol));
}

// 45 degrees symmetric fov
TEST(VrMath, ProjectionSymmetric45) {
    float a = std::tan(static_cast<float>(M_PI) / 4.f); // tan(45°) = 1
    float near_z = 0.1f;
    float far_z  = 100.f;
    XrFovf fov{
        .angleLeft  = -static_cast<float>(M_PI) / 4.f,
        .angleRight =  static_cast<float>(M_PI) / 4.f,
        .angleUp    =  static_cast<float>(M_PI) / 4.f,
        .angleDown  = -static_cast<float>(M_PI) / 4.f,
    };
    auto m = projection(fov, near_z, far_z);

    // l=-near, r=near, b=-near, t=near
    // m(0,0) = 2*near/(r-l) = 2*near/(2*near) = 1
    EXPECT_NEAR(m(0,0), 1.f, kTol);
    // m(1,1) = 1 same
    EXPECT_NEAR(m(1,1), 1.f, kTol);
    // symmetric: offsets zero
    EXPECT_NEAR(m(0,2), 0.f, kTol);
    EXPECT_NEAR(m(1,2), 0.f, kTol);
    // perspective divide row
    EXPECT_NEAR(m(3,2), -1.f, kTol);
    EXPECT_NEAR(m(3,3),  0.f, kTol);
    // depth terms
    float expected_22 = -(far_z + near_z) / (far_z - near_z);
    float expected_23 = -2.f * far_z * near_z / (far_z - near_z);
    EXPECT_NEAR(m(2,2), expected_22, kTol);
    EXPECT_NEAR(m(2,3), expected_23, kTol);
}

// 90° rotation around Y: quaternion (0, sin45, 0, cos45)
TEST(VrMath, ViewRotation90Y) {
    float s = std::sqrt(0.5f);
    XrPosef pose{
        .orientation = {0.f, s, 0.f, s},  // 90° around Y
        .position    = {0.f, 0.f, 0.f},
    };
    // R_y(90): X->-Z, Y->Y, Z->X
    // R^T (view rotation):
    //   col0 (new X): ( 0,0,-1)
    //   col1 (new Y): ( 0,1, 0)
    //   col2 (new Z): ( 1,0, 0)
    auto m = view(pose);
    // Row 0 of Rt = col 0 of R: (0,0,1,0) — R_y(90) col0 = (0,0,-1), so row0 of Rt = (0,0,-1)
    // Actually R_y(90°): [[0,0,1],[0,1,0],[-1,0,0]] — let's check transposed
    // R_y(90): col0=(0,0,-1), col1=(0,1,0), col2=(1,0,0)
    // Rt row0 = R col0^T = (0,0,-1)
    EXPECT_NEAR(m(0,0),  0.f, kTol);
    EXPECT_NEAR(m(0,1),  0.f, kTol);
    EXPECT_NEAR(m(0,2), -1.f, kTol);
    EXPECT_NEAR(m(1,0),  0.f, kTol);
    EXPECT_NEAR(m(1,1),  1.f, kTol);
    EXPECT_NEAR(m(1,2),  0.f, kTol);
    EXPECT_NEAR(m(2,0),  1.f, kTol);
    EXPECT_NEAR(m(2,1),  0.f, kTol);
    EXPECT_NEAR(m(2,2),  0.f, kTol);
    // no translation
    EXPECT_NEAR(m(0,3),  0.f, kTol);
    EXPECT_NEAR(m(1,3),  0.f, kTol);
    EXPECT_NEAR(m(2,3),  0.f, kTol);
}
