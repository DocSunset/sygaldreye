// Copyright 2025 Travis West
#include "water_surface.hpp"
#include "egl_context.hpp"
#include <gtest/gtest.h>
#include <cmath>

class WaterSurfaceTest : public ::testing::Test {
protected:
    EglContext ctx_;
    void SetUp() override { ASSERT_TRUE(ctx_.init()); }
};

TEST_F(WaterSurfaceTest, UpdateProducesDifferentHeights) {
    WaterParams p;
    p.grid_w = 4;
    p.grid_h = 4;
    auto w = WaterSurface::create(p);

    w.update(0.0f);
    float h0 = w.mesh_data().vertices[0].position.y();

    w.update(1.0f);
    float h1 = w.mesh_data().vertices[0].position.y();

    EXPECT_NE(h0, h1);
}

TEST_F(WaterSurfaceTest, NormalsRemainUnitLength) {
    WaterParams p;
    p.grid_w = 4;
    p.grid_h = 4;
    auto w = WaterSurface::create(p);
    w.update(0.5f);

    for (auto const& v : w.mesh_data().vertices) {
        EXPECT_NEAR(v.normal.norm(), 1.0f, 1e-5f);
    }
}
