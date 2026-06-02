#include "scene.hpp"
#include <gtest/gtest.h>
#include <cmath>

TEST(SceneTest, UpdateProducesOneCube) {
    Scene s;
    s.update(0.0);
    EXPECT_EQ(s.cubes().size(), 1u);
}

TEST(SceneTest, ModelChangesOverTime) {
    Scene s;
    s.update(0.0);
    Eigen::Matrix4f m0 = s.cubes()[0].model;
    s.update(10.0);
    Eigen::Matrix4f m1 = s.cubes()[0].model;
    EXPECT_FALSE(m0.isApprox(m1, 1e-4f));
}

TEST(SceneTest, CubePlacement) {
    Scene s;
    s.update(0.0);
    auto m = s.cubes()[0].model;
    EXPECT_NEAR(m(0, 3), 0.0f, 1e-4f);
    EXPECT_NEAR(m(1, 3), 1.5f, 1e-4f);
    EXPECT_NEAR(m(2, 3), -5.0f, 1e-4f);
}
