// Copyright 2025 Travis West
#include "hrtf_spatializer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <synth_core.hpp>

static constexpr int kFrames = 512;

static std::array<float, kFrames> make_sine(float freq, float sr) {
    std::array<float, kFrames> buf{};
    synth::Phasor p;
    p.freq = freq;
    p.sample_rate = sr;
    for (auto& s : buf) s = synth::sine(p.tick());
    return buf;
}

TEST(HrtfSpatializer, LeftSourceRightChannelLags) {
    HrtfSpatializer spat;
    spat.set_position({-1.0f, 0.0f, 0.0f}, 1.0f);

    auto mono = make_sine(1000.0f, 48000.0f);
    std::array<float, kFrames * 2> stereo{};
    spat.process(mono.data(), stereo.data(), kFrames);

    // Find peak index in each channel
    int peak_l = 0, peak_r = 0;
    float max_l = 0.0f, max_r = 0.0f;
    for (int i = 0; i < kFrames; ++i) {  // planar: [L block][R block]
        float l = std::abs(stereo[i]);
        float r = std::abs(stereo[kFrames + i]);
        if (l > max_l) {
            max_l = l;
            peak_l = i;
        }
        if (r > max_r) {
            max_r = r;
            peak_r = i;
        }
    }
    // Source is left → right channel is contralateral (delayed) → peak_r > peak_l
    EXPECT_GT(peak_r, peak_l);
}

TEST(HrtfSpatializer, FrontSourceEqualAmplitude) {
    HrtfSpatializer spat;
    spat.set_position({0.0f, 0.0f, -1.0f}, 1.0f);

    auto mono = make_sine(1000.0f, 48000.0f);
    std::array<float, kFrames * 2> stereo{};
    spat.process(mono.data(), stereo.data(), kFrames);

    float rms_l = 0.0f, rms_r = 0.0f;
    for (int i = 0; i < kFrames; ++i) {  // planar: [L block][R block]
        rms_l += stereo[i] * stereo[i];
        rms_r += stereo[kFrames + i] * stereo[kFrames + i];
    }
    rms_l = std::sqrt(rms_l / kFrames);
    rms_r = std::sqrt(rms_r / kFrames);

    ASSERT_GT(rms_l, 0.0f);
    EXPECT_NEAR(rms_l, rms_r, 0.01f * rms_l);
}
