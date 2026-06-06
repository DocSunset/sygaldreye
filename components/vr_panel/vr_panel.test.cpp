// Copyright 2025 Travis West
#include "vr_panel.hpp"
#include <gtest/gtest.h>

TEST(VrPanel, HitFrontFace) {
    VrPanel p;
    p.position = {0, 0, -1};
    p.normal   = {0, 0,  1};
    p.width    = 0.4f;
    p.height   = 0.3f;

    auto hit = p.intersect({0, 0, 0}, {0, 0, -1});
    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->t, 1.0f, 1e-5f);
    EXPECT_NEAR(hit->uv.x(), 0.5f, 1e-5f);
    EXPECT_NEAR(hit->uv.y(), 0.5f, 1e-5f);
}

TEST(VrPanel, MissWrongDirection) {
    VrPanel p;
    p.position = {0, 0, -1};
    p.normal   = {0, 0,  1};
    auto hit = p.intersect({0, 0, 0}, {0, 0, 1});
    EXPECT_FALSE(hit.has_value());
}

TEST(VrPanel, MissOutsideBounds) {
    VrPanel p;
    p.position = {0, 0, -1};
    p.normal   = {0, 0,  1};
    p.width    = 0.4f;
    p.height   = 0.3f;

    auto hit = p.intersect({0, 0, 0}, {1, 0, -1});
    EXPECT_FALSE(hit.has_value());
}

TEST(VrPanel, HitEdge) {
    VrPanel p;
    p.position = {0, 0, -2};
    p.normal   = {0, 0,  1};
    p.width    = 1.0f;
    p.height   = 1.0f;
    // Ray aimed at just inside the right edge
    auto hit = p.intersect({0.49f, 0, 0}, {0, 0, -1});
    ASSERT_TRUE(hit.has_value());
    EXPECT_GT(hit->uv.x(), 0.9f);
}
