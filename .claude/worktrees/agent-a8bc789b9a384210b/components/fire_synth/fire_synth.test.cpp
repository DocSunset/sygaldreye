// Copyright 2025 Travis West
#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "fire_synth.hpp"

static float rms(float const* buf, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += buf[i] * buf[i];
    return std::sqrt(sum / static_cast<float>(n));
}

TEST(FireSynth, SilentAtIntensityZero) {
    FireParams p;
    p.intensity = 0.0f;
    FireSynth synth(p);
    std::vector<float> buf(512, 0.0f);
    synth.fill(buf.data(), static_cast<int>(buf.size()));
    for (int i = 0; i < 512; ++i)
        EXPECT_NEAR(buf[i], 0.0f, 0.01f);
}

TEST(FireSynth, HigherIntensityProducesHigherRms) {
    std::vector<float> lo(2048, 0.0f);
    std::vector<float> hi(2048, 0.0f);

    FireParams lp; lp.intensity = 1.0f;
    FireSynth lo_synth(lp);
    lo_synth.fill(lo.data(), static_cast<int>(lo.size()));

    FireParams hp; hp.intensity = 2.0f;
    FireSynth hi_synth(hp);
    hi_synth.fill(hi.data(), static_cast<int>(hi.size()));

    EXPECT_GT(rms(hi.data(), static_cast<int>(hi.size())),
              rms(lo.data(), static_cast<int>(lo.size())));
}
