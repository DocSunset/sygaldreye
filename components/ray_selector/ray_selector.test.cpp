// Copyright 2025 Travis West
#include "ray_selector.hpp"
#include <gtest/gtest.h>
#include <cmath>

static XrPosef make_pose(float px, float py, float pz,
                          float qx, float qy, float qz, float qw) {
    XrPosef p{};
    p.position    = {px, py, pz};
    p.orientation = {qx, qy, qz, qw};
    return p;
}

// Identity rotation: looking along -Z
static XrPosef identity_at(float x, float y, float z) {
    return make_pose(x, y, z, 0, 0, 0, 1);
}

TEST(RaySelector, HitsNearestPanel) {
    VrPanel a, b;
    a.position = {0, 0, -1};
    a.normal   = {0, 0,  1};
    a.width = a.height = 1.0f;

    b.position = {0, 0, -2};
    b.normal   = {0, 0,  1};
    b.width = b.height = 1.0f;

    std::vector<VrPanel> panels = {a, b};
    auto hit = RaySelector::test(identity_at(0, 0, 0), panels);
    ASSERT_TRUE(hit.has_value());
    EXPECT_EQ(hit->panel_index, 0);
}

TEST(RaySelector, MissesAllPanels) {
    VrPanel p;
    p.position = {0, 0, -1};
    p.normal   = {0, 0,  1};
    p.width = p.height = 0.1f;

    std::vector<VrPanel> panels = {p};
    // Ray from far off-axis
    auto hit = RaySelector::test(identity_at(5, 0, 0), panels);
    EXPECT_FALSE(hit.has_value());
}

TEST(RaySelector, EmptyPanelList) {
    auto hit = RaySelector::test(identity_at(0, 0, 0), {});
    EXPECT_FALSE(hit.has_value());
}
