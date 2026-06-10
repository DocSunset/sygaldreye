#include "atmos_synth.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <numeric>

static float rms(float const* buf, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += buf[i] * buf[i];
    return std::sqrt(sum / static_cast<float>(n));
}

TEST(AtmosSynth, ZeroWindSpeedNearSilence) {
    AtmosParams p;
    p.wind_speed = 0.0f;
    AtmosSynth s(p);
    float buf[512] = {};
    s.fill(buf, 512);
    for (int i = 0; i < 512; ++i)
        EXPECT_LT(std::abs(buf[i]), 0.01f);
}

TEST(AtmosSynth, HigherWindSpeedHigherRms) {
    float buf0[512] = {};
    float buf1[512] = {};

    AtmosParams p0;
    p0.wind_speed = 0.0f;
    AtmosSynth s0(p0);
    s0.fill(buf0, 512);

    AtmosParams p1;
    p1.wind_speed = 1.0f;
    AtmosSynth s1(p1);
    s1.fill(buf1, 512);

    EXPECT_GT(rms(buf1, 512), rms(buf0, 512));
}
