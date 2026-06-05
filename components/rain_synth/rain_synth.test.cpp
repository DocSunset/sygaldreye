// Copyright 2025 Travis West
#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "rain_synth.hpp"

static float rms(float const* buf, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += buf[i] * buf[i];
    return std::sqrt(sum / static_cast<float>(n));
}

TEST(RainSynth, SilentAtRateZero) {
    RainParams p;
    p.rate = 0.0f;
    RainSynth synth(p);
    std::vector<float> buf(512, 0.0f);
    synth.fill(buf.data(), static_cast<int>(buf.size()));
    EXPECT_NEAR(rms(buf.data(), static_cast<int>(buf.size())), 0.0f, 1e-6f);
}

TEST(RainSynth, HigherRateProducesHigherRms) {
    std::vector<float> light(2048, 0.0f);
    std::vector<float> heavy(2048, 0.0f);

    RainParams lp; lp.rate = 200.0f;
    RainSynth light_synth(lp);
    light_synth.fill(light.data(), static_cast<int>(light.size()));

    RainParams hp; hp.rate = 2000.0f;
    RainSynth heavy_synth(hp);
    heavy_synth.fill(heavy.data(), static_cast<int>(heavy.size()));

    EXPECT_GT(rms(heavy.data(), static_cast<int>(heavy.size())),
              rms(light.data(), static_cast<int>(light.size())));
}
