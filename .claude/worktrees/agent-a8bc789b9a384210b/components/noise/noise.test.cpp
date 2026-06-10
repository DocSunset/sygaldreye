#include "noise.hpp"
#include <gtest/gtest.h>
#include <cmath>

TEST(Noise, Perlin2dRange) {
    for (int i = 0; i < 100; ++i) {
        float x = i * 0.13f;
        float y = i * 0.07f;
        float v = noise::perlin(x, y);
        EXPECT_GE(v, -1.5f);
        EXPECT_LE(v,  1.5f);
    }
}

TEST(Noise, Perlin3dRange) {
    for (int i = 0; i < 100; ++i) {
        float x = i * 0.13f, y = i * 0.07f, z = i * 0.11f;
        float v = noise::perlin(x, y, z);
        EXPECT_GE(v, -1.5f);
        EXPECT_LE(v,  1.5f);
    }
}

TEST(Noise, Reproducible) {
    EXPECT_FLOAT_EQ(noise::perlin(1.23f, 4.56f), noise::perlin(1.23f, 4.56f));
    EXPECT_FLOAT_EQ(noise::perlin(1.f, 2.f, 3.f), noise::perlin(1.f, 2.f, 3.f));
}

TEST(Noise, Smooth2d) {
    float step = 0.01f;
    float bound = 2.f * step;
    for (int i = 0; i < 50; ++i) {
        float x = i * 0.1f;
        float y = 0.5f;
        float diff = std::abs(noise::perlin(x + step, y) - noise::perlin(x, y));
        EXPECT_LT(diff, bound);
    }
}

TEST(Noise, FbmOneOctaveEqualsPerlin) {
    for (int i = 0; i < 20; ++i) {
        float x = i * 0.17f, y = i * 0.23f;
        EXPECT_FLOAT_EQ(noise::fbm(x, y, 1, 2.f, 0.5f), noise::perlin(x, y));
    }
}
