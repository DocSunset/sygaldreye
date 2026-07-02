// Copyright 2026 Travis West
#include "sprite.hpp"

#include <gtest/gtest.h>

TEST(Sprite, QuadIsBuiltOnceWithStableVersion) {
    SpriteNode n;
    n(0.0);
    const auto* geo = n.endpoints.mesh.value.geometry.get();
    ASSERT_NE(geo, nullptr);
    EXPECT_EQ(geo->vertices.size(), 4u);
    EXPECT_EQ(geo->indices.size(), 6u);
    const auto v = geo->version;
    n(0.1);
    // Same quad, same version ⇒ render_region uploads it exactly once.
    EXPECT_EQ(n.endpoints.mesh.value.geometry.get(), geo);
    EXPECT_EQ(n.endpoints.mesh.value.geometry->version, v);
}

TEST(Sprite, BlendedNonOccludingSurface) {
    SpriteNode n;
    n(0.0);
    const Surface& s = n.endpoints.surface.value;
    EXPECT_TRUE(s.blend);
    EXPECT_FALSE(s.depth_write);
    EXPECT_FALSE(s.cull_back);
    EXPECT_TRUE(s.shader);
}

TEST(Sprite, PositionsSpanDrivesInstances) {
    SpriteNode n;
    float pts[9] = {0, 0, 0, 1, 1, 1, 2, 2, 2};
    Span s{pts, 3, 3, Axis::Item, Axis::Cell};
    n.endpoints.positions.src = &s;
    n(0.0);
    ASSERT_EQ(n.endpoints.mesh.value.instances.size(), 1u);
    EXPECT_EQ(n.endpoints.mesh.value.instances[0].data.rows, 3);
}
