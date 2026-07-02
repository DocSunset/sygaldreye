// Copyright 2026 Travis West
#include "vertex_color_mesh.hpp"

#include <gtest/gtest.h>

#include "tri_mesh.hpp"

namespace {
std::shared_ptr<TriMeshData> a_grid() {
    auto m = std::make_shared<TriMeshData>();
    m->vertices = {
        {{0, 0, 0}, {0, 1, 0}, {1, 0, 0, 1}},
        {{1, 0, 0}, {0, 1, 0}, {0, 1, 0, 1}},
        {{0, 0, 1}, {0, 1, 0}, {0, 0, 1, 1}}};
    m->indices = {0, 1, 2};
    return m;
}
}  // namespace

TEST(VertexColorMesh, EmitsSunUniformsAndDynamicHint) {
    VertexColorMeshNode n;
    MeshPtr g = a_grid();
    n.endpoints.geometry.src = &g;
    n(0.0);
    EXPECT_EQ(n.endpoints.mesh.value.geometry, g);
    EXPECT_TRUE(n.endpoints.mesh.value.dynamic);
    ASSERT_EQ(n.endpoints.surface.value.uniforms.size(), 3u);
    EXPECT_EQ(n.endpoints.surface.value.uniforms[0].name, "uSunDir");
}

TEST(VertexColorMesh, TouchedGeometryCarriesANewVersion) {
    // The version contract producers rely on: mutate in place + touch ⇒ the
    // boundary sees a new stamp through the same pointer.
    VertexColorMeshNode n;
    auto grid = a_grid();
    MeshPtr g = grid;
    n.endpoints.geometry.src = &g;
    n(0.0);
    const auto v0 = n.endpoints.mesh.value.geometry->version;
    grid->vertices[0].color = {1, 1, 0, 1};
    grid->touch();
    n(0.1);
    EXPECT_EQ(n.endpoints.mesh.value.geometry.get(), grid.get());
    EXPECT_GT(n.endpoints.mesh.value.geometry->version, v0);
}

TEST(VertexColorMesh, WiredSunOverridesDefault) {
    VertexColorMeshNode n;
    MeshPtr g = a_grid();
    n.endpoints.geometry.src = &g;
    Eigen::Vector3f dir{1.f, 0.f, 0.f};
    n.endpoints.sun_dir.src = &dir;
    n(0.0);
    auto v = std::get<Eigen::Vector3f>(n.endpoints.surface.value.uniforms[0].value);
    EXPECT_FLOAT_EQ(v.x(), 1.f);
}
