// Copyright 2025 Travis West
#include <gtest/gtest.h>
#include <cmath>
#include "water_synth.hpp"

TEST(WaterSynth, FlowSpeedZeroIsNearSilent) {
    WaterParams p;
    p.flow_speed  = 0.0f;
    p.wave_height = 0.0f;
    WaterSynth synth(p);
    float buf[512] = {};
    synth.fill(buf, 512);
    float sum_sq = 0.0f;
    for (float s : buf) sum_sq += s * s;
    EXPECT_LT(std::sqrt(sum_sq / 512), 1e-4f);
}

TEST(WaterSynth, WaveRateModulatesAmplitude) {
    constexpr int sr        = 48000;
    constexpr int total     = 5 * sr;
    constexpr int blocks    = 10;
    constexpr int block_len = total / blocks;

    WaterParams p;
    p.wave_rate   = 0.2f;
    p.wave_height = 0.8f;
    p.sample_rate = static_cast<float>(sr);
    WaterSynth synth(p);

    static float buf[total];
    synth.fill(buf, total);

    float rms[blocks];
    for (int b = 0; b < blocks; ++b) {
        float sum = 0.0f;
        for (int i = 0; i < block_len; ++i) {
            float s = buf[b * block_len + i];
            sum += s * s;
        }
        rms[b] = std::sqrt(sum / static_cast<float>(block_len));
    }

    float mean = 0.0f;
    for (float r : rms) mean += r;
    mean /= static_cast<float>(blocks);

    float variance = 0.0f;
    for (float r : rms) {
        float d = r - mean;
        variance += d * d;
    }
    variance /= static_cast<float>(blocks);

    EXPECT_GT(variance, 0.001f);
}
