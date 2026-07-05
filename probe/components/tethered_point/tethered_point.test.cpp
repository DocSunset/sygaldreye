#include "tethered_point.hpp"
#include <gtest/gtest.h>

static constexpr float kTol = 1e-5f;

TEST(TetheredPoint, WithinRange) {
    Eigen::Vector3f anchor(0.f, 0.f, 0.f);
    Eigen::Vector3f desired(3.f, 4.f, 0.f);  // distance = 5
    float min_dist = 2.f;
    float max_dist = 10.f;

    Eigen::Vector3f result = tethered_point(anchor, desired, min_dist, max_dist);

    EXPECT_TRUE(result.isApprox(desired, kTol));
}

TEST(TetheredPoint, TooClose) {
    Eigen::Vector3f anchor(0.f, 0.f, 0.f);
    Eigen::Vector3f desired(1.f, 0.f, 0.f);  // distance = 1
    float min_dist = 2.f;
    float max_dist = 10.f;

    Eigen::Vector3f result = tethered_point(anchor, desired, min_dist, max_dist);

    // Expected: anchor + min_dist * unit_direction
    // direction = (1, 0, 0) / 1 = (1, 0, 0)
    // result = (0, 0, 0) + 2 * (1, 0, 0) = (2, 0, 0)
    Eigen::Vector3f expected(2.f, 0.f, 0.f);
    EXPECT_TRUE(result.isApprox(expected, kTol));
}

TEST(TetheredPoint, TooFar) {
    Eigen::Vector3f anchor(0.f, 0.f, 0.f);
    Eigen::Vector3f desired(30.f, 0.f, 0.f);  // distance = 30
    float min_dist = 2.f;
    float max_dist = 10.f;

    Eigen::Vector3f result = tethered_point(anchor, desired, min_dist, max_dist);

    // Expected: anchor + max_dist * unit_direction
    // direction = (30, 0, 0) / 30 = (1, 0, 0)
    // result = (0, 0, 0) + 10 * (1, 0, 0) = (10, 0, 0)
    Eigen::Vector3f expected(10.f, 0.f, 0.f);
    EXPECT_TRUE(result.isApprox(expected, kTol));
}

TEST(TetheredPoint, Coincident) {
    Eigen::Vector3f anchor(5.f, 5.f, 5.f);
    Eigen::Vector3f desired(5.f, 5.f, 5.f);  // same as anchor
    float min_dist = 3.f;
    float max_dist = 10.f;

    Eigen::Vector3f result = tethered_point(anchor, desired, min_dist, max_dist);

    // Should not crash and result should be min_dist away from anchor
    // Degenerate case: arbitrary direction is UnitX()
    // result = (5, 5, 5) + 3 * (1, 0, 0) = (8, 5, 5)
    Eigen::Vector3f expected(8.f, 5.f, 5.f);
    EXPECT_TRUE(result.isApprox(expected, kTol));
}
