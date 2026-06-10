// Copyright 2025 Travis West
#include <gtest/gtest.h>
#include <array>
#include <cmath>
#include "chime_synth.hpp"

TEST(ChimeSynth, StrikeProducesNonZeroOutput)
{
    ChimeSynth synth{wind_chime_params()};
    synth.strike(1.0f);
    std::array<float, 512> buf{};
    synth.fill(buf.data(), 512);
    float sum = 0.0f;
    for (float s : buf) sum += s * s;
    EXPECT_GT(sum, 0.0f);
}

TEST(ChimeSynth, OutputDecaysToNearZeroAfterLongestDecay)
{
    ChimeParams p = wind_chime_params();
    ChimeSynth synth{p};
    synth.strike(1.0f);

    constexpr float sample_rate = 48000.0f;
    constexpr int   duration_s  = 5;
    constexpr int   total       = static_cast<int>(sample_rate * duration_s);
    std::array<float, 1000> tail{};
    std::array<float, 4096> block{};

    int remaining = total - 1000;
    while (remaining > 0) {
        int n = remaining < 4096 ? remaining : 4096;
        synth.fill(block.data(), n);
        remaining -= n;
    }
    synth.fill(tail.data(), 1000);

    float rms = 0.0f;
    for (float s : tail) rms += s * s;
    rms = std::sqrt(rms / 1000.0f);
    EXPECT_LT(rms, 0.01f);
}

TEST(ChimeSynth, NoStrikeNoOutput)
{
    ChimeParams p = wind_chime_params();
    p.strike_rate = 0.0f;
    ChimeSynth synth{p};
    std::array<float, 512> buf{};
    synth.fill(buf.data(), 512);
    for (float s : buf) EXPECT_FLOAT_EQ(s, 0.0f);
}
