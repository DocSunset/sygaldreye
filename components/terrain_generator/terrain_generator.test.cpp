// Copyright 2025 Travis West
#include "terrain_generator.hpp"
#include <gtest/gtest.h>
#include <cmath>

static TerrainParams default_params() {
    TerrainParams p;
    p.band_colors = {
        Eigen::Vector4f{0.0f, 0.2f, 0.8f, 1.0f},
        Eigen::Vector4f{0.1f, 0.6f, 0.1f, 1.0f},
        Eigen::Vector4f{0.4f, 0.3f, 0.2f, 1.0f},
        Eigen::Vector4f{1.0f, 1.0f, 1.0f, 1.0f},
    };
    return p;
}

TEST(TerrainGenerator, VertexCount) {
    auto p    = default_params();
    p.grid_w  = 8;
    p.grid_h  = 6;
    auto mesh = generate_terrain(p);
    int expected_faces = (p.grid_w - 1) * (p.grid_h - 1) * 2;
    EXPECT_EQ(static_cast<int>(mesh.vertices.size()), expected_faces * 3);
}

TEST(TerrainGenerator, IndexCount) {
    auto p   = default_params();
    p.grid_w = 8;
    p.grid_h = 6;
    auto mesh = generate_terrain(p);
    int expected = (p.grid_w - 1) * (p.grid_h - 1) * 2 * 3;
    EXPECT_EQ(static_cast<int>(mesh.indices.size()), expected);
}

TEST(TerrainGenerator, NormalsUnitLength) {
    auto p    = default_params();
    p.grid_w  = 8;
    p.grid_h  = 8;
    auto mesh = generate_terrain(p);
    for (auto const& v : mesh.vertices) {
        float len = v.normal.norm();
        EXPECT_NEAR(len, 1.0f, 1e-5f);
    }
}
