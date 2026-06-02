#include "scene.hpp"
#include <gtest/gtest.h>
#include <cmath>

TEST(SceneTest, UpdateProducesOneCube) {
    Scene s;
    s.update(0.0);
    EXPECT_EQ(s.cubes().size(), 1u);
}

TEST(SceneTest, RotationAngleIncreases) {
    Scene s;
    s.update(0.0);
    float cos0 = s.cubes()[0].model(0, 0);
    EXPECT_NEAR(cos0, 1.0f, 1e-4f);

    s.update(M_PI / 2.0);
    float cosHalfPi = s.cubes()[0].model(0, 0);
    EXPECT_NEAR(cosHalfPi, 0.0f, 1e-4f);
}

TEST(SceneTest, CubePlacement) {
    Scene s;
    s.update(0.0);
    auto m = s.cubes()[0].model;
    EXPECT_NEAR(m(0, 3), 0.0f, 1e-4f);
    EXPECT_NEAR(m(1, 3), 1.5f, 1e-4f);
    EXPECT_NEAR(m(2, 3), -2.0f, 1e-4f);
}
