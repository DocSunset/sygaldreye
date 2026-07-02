// Copyright 2026 Travis West
#include "color_mesh.hpp"

#include <gtest/gtest.h>

#include "tri_mesh.hpp"

namespace {
MeshPtr a_triangle() {
    auto m = std::make_shared<TriMeshData>();
    m->vertices = {
        {{0, 0, 0}, {0, 0, 1}, {1, 1, 1, 1}},
        {{1, 0, 0}, {0, 0, 1}, {1, 1, 1, 1}},
        {{0, 1, 0}, {0, 0, 1}, {1, 1, 1, 1}}};
    m->indices = {0, 1, 2};
    return m;
}
}  // namespace

TEST(ColorMesh, WrapsGeometryWithStableIdentity) {
    ColorMeshNode n;
    MeshPtr g = a_triangle();
    n.endpoints.geometry.src = &g;
    n(0.0);
    EXPECT_EQ(n.endpoints.mesh.value.geometry, g);  // passthrough, same version
    const auto v = n.endpoints.mesh.value.geometry->version;
    n(0.1);
    EXPECT_EQ(n.endpoints.mesh.value.geometry->version, v);  // no touch, no re-upload
    EXPECT_TRUE(n.endpoints.surface.value.shader);
    ASSERT_EQ(n.endpoints.surface.value.uniforms.size(), 1u);
    EXPECT_EQ(n.endpoints.surface.value.uniforms[0].name, "uColor");
}

TEST(ColorMesh, UnwiredPositionsGiveOneOriginInstance) {
    ColorMeshNode n;
    MeshPtr g = a_triangle();
    n.endpoints.geometry.src = &g;
    n(0.0);
    ASSERT_EQ(n.endpoints.mesh.value.instances.size(), 1u);
    EXPECT_EQ(n.endpoints.mesh.value.instances[0].data.rows, 1);
    EXPECT_EQ(n.endpoints.mesh.value.instances[0].data.cols, 3);
}

TEST(ColorMesh, NRowSpanBecomesNInstances) {
    ColorMeshNode n;
    MeshPtr g = a_triangle();
    n.endpoints.geometry.src = &g;
    float pts[6] = {0, 0, 0, 1, 0, 0};
    Span s{pts, 2, 3, Axis::Item, Axis::Cell};
    n.endpoints.positions.src = &s;
    n(0.0);
    ASSERT_EQ(n.endpoints.mesh.value.instances.size(), 1u);
    EXPECT_EQ(n.endpoints.mesh.value.instances[0].data.rows, 2);
    EXPECT_EQ(n.endpoints.mesh.value.instances[0].name, "aOffset");
}
