// Copyright 2026 Travis West
#include "card_mesh.hpp"

#include <gtest/gtest.h>

TEST(CardMesh, QuadAtPosition) {
    CardMeshNode c;
    Eigen::Vector3f p{0.5f, 1.2f, -0.5f};
    c.endpoints.position.src = &p;
    c(0.0);
    auto m = c.endpoints.mesh.value;
    ASSERT_TRUE(m);
    EXPECT_EQ(m->vertices.size(), 4u);
    EXPECT_EQ(m->indices.size(), 6u);
    // Quad is centered on the position, in the z=p.z plane.
    for (const auto& v : m->vertices) EXPECT_FLOAT_EQ(v.position.z(), -0.5f);
    Eigen::Vector3f centroid = Eigen::Vector3f::Zero();
    for (const auto& v : m->vertices) centroid += v.position;
    centroid /= 4.f;
    EXPECT_NEAR(centroid.x(), 0.5f, 1e-5f);
    EXPECT_NEAR(centroid.y(), 1.2f, 1e-5f);
}

TEST(CardMesh, CachesUntilMoved) {
    CardMeshNode c;
    Eigen::Vector3f p{0, 0, 0};
    c.endpoints.position.src = &p;
    c(0.0);
    auto m1 = c.endpoints.mesh.value;
    c(0.0);
    EXPECT_EQ(c.endpoints.mesh.value, m1);  // unchanged → same shared mesh
    p = {1, 0, 0};
    c(0.0);
    EXPECT_NE(c.endpoints.mesh.value, m1);  // moved → new mesh
}
