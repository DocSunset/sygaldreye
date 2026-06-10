// Copyright 2025 Travis West
#include <gtest/gtest.h>
#include "engine_synth.hpp"
#include <vector>
#include <cmath>

static int count_zero_crossings(float const* buf, int n) {
    int count = 0;
    for (int i = 1; i < n; ++i)
        if ((buf[i - 1] < 0.0f) != (buf[i] < 0.0f))
            ++count;
    return count;
}

TEST(EngineSynth, ZeroCrossingsMatchFundamental) {
    EngineParams p;
    p.rpm         = 800.0f;
    p.cylinders   = 4;
    p.sample_rate = 48000.0f;
    EngineSynth synth(p);

    constexpr int frames = 48000;
    std::vector<float> buf(frames);
    synth.fill(buf.data(), frames);

    // fund_hz = 800/60 * 4/2 = 26.67 Hz → ~53 crossings per second
    float expected = 2.0f * (p.rpm / 60.0f * p.cylinders / 2.0f);
    int   crossings = count_zero_crossings(buf.data(), frames);
    EXPECT_GT(crossings, static_cast<int>(expected * 0.8f));
    EXPECT_LT(crossings, static_cast<int>(expected * 1.2f));
}

TEST(EngineSynth, HigherRpmMoreCrossings) {
    constexpr int frames = 48000;
    std::vector<float> buf(frames);

    EngineParams lo;
    lo.rpm = 800.0f;
    lo.cylinders = 4;
    lo.sample_rate = 48000.0f;
    EngineSynth slow(lo);
    slow.fill(buf.data(), frames);
    int crossings_lo = count_zero_crossings(buf.data(), frames);

    EngineParams hi;
    hi.rpm = 4000.0f;
    hi.cylinders = 4;
    hi.sample_rate = 48000.0f;
    EngineSynth fast(hi);
    fast.fill(buf.data(), frames);
    int crossings_hi = count_zero_crossings(buf.data(), frames);

    EXPECT_GT(crossings_hi, crossings_lo);
}
