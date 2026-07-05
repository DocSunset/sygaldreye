// Copyright 2026 Travis West
#include "flat_shader.hpp"

#include <gtest/gtest.h>

#include "tri_mesh.hpp"

static MeshPtr a_triangle() {
    auto m = std::make_shared<TriMeshData>();
    m->vertices = {
        {{0, 0, 0}, {0, 0, 1}, {1, 0, 0, 1}},
        {{1, 0, 0}, {0, 0, 1}, {0, 1, 0, 1}},
        {{0, 1, 0}, {0, 0, 1}, {0, 0, 1, 1}}};
    m->indices = {0, 1, 2};
    return m;
}

TEST(FlatMesh, PassesGeometryAndEmitsSurface) {
    FlatMeshNode n;
    MeshPtr g = a_triangle();
    n.endpoints.geometry.src = &g;
    n(0.0);
    EXPECT_EQ(n.endpoints.mesh.value.geometry, g);  // geometry passed through
    EXPECT_TRUE(n.endpoints.surface.value.shader);
    EXPECT_EQ(n.endpoints.mesh.value.instances.size(), 1u);  // one default offset
}

TEST(FlatMesh, TintIsAUniform) {
    FlatMeshNode n;
    MeshPtr g = a_triangle();
    n.endpoints.geometry.src = &g;
    n.endpoints.tint_r.fallback = 0.5f;
    n(0.0);
    ASSERT_EQ(n.endpoints.surface.value.uniforms.size(), 1u);
    EXPECT_EQ(n.endpoints.surface.value.uniforms[0].name, "uTint");
    auto v = std::get<Eigen::Vector4f>(n.endpoints.surface.value.uniforms[0].value);
    EXPECT_FLOAT_EQ(v.x(), 0.5f);
}
