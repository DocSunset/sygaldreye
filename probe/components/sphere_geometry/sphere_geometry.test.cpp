// Copyright 2025 Travis West
#include "sphere_geometry.hpp"
#include <gtest/gtest.h>
#include <cmath>

TEST(SphereGeometry, VertexCount) {
    auto geo = make_sphere(32, 16);
    EXPECT_EQ(geo.vertices.size(), 33u * 17u);
}

TEST(SphereGeometry, IndexCount) {
    auto geo = make_sphere(32, 16);
    EXPECT_EQ(geo.indices.size(), 32u * 16u * 6u);
}

TEST(SphereGeometry, IndicesInRange) {
    auto geo = make_sphere(32, 16);
    for (uint16_t idx : geo.indices)
        EXPECT_LT(idx, geo.vertices.size());
}

TEST(SphereGeometry, NormalsUnitLength) {
    auto geo = make_sphere(32, 16);
    for (auto& v : geo.vertices)
        EXPECT_NEAR(v.normal.norm(), 1.0f, 1e-5f);
}
