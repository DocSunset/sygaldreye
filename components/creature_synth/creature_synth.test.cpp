// Copyright 2025 Travis West
#include "creature_synth.hpp"
#include <gtest/gtest.h>
#include <cmath>
#include <vector>

TEST(CreatureSynth, CricketHasSilentGaps) {
    CreatureParams p;
    p.kind        = CreatureKind::Cricket;
    p.sample_rate = 48000.0f;
    p.chirp_rate  = 20.0f;
    CreatureSynth synth{p};

    int frames = static_cast<int>(2.0f * p.sample_rate);
    std::vector<float> buf(frames, 0.0f);
    synth.fill(buf.data(), frames);

    int active  = 0;
    int silent  = 0;
    for (float s : buf) {
        if (std::abs(s) > 0.01f)  ++active;
        if (std::abs(s) < 0.001f) ++silent;
    }
    EXPECT_GT(active, 0);
    EXPECT_GT(silent, frames / 4);
}

TEST(CreatureSynth, BirdPitchContourVaries) {
    CreatureParams p;
    p.kind             = CreatureKind::Bird;
    p.sample_rate      = 48000.0f;
    p.phrase_duration_s = 2.0f;
    CreatureSynth synth{p};

    int frames   = static_cast<int>(p.phrase_duration_s * p.sample_rate);
    int quarter  = frames / 4;
    std::vector<float> buf(frames, 0.0f);
    synth.fill(buf.data(), frames);

    auto zero_crossings = [&](int start, int end) {
        int zc = 0;
        for (int i = start + 1; i < end; ++i)
            if (buf[i - 1] * buf[i] < 0.0f) ++zc;
        return zc;
    };

    int zc_first = zero_crossings(0, quarter);
    int zc_last  = zero_crossings(frames - quarter, frames);

    float ratio = (zc_first > 0 && zc_last > 0)
        ? static_cast<float>(std::abs(zc_first - zc_last)) / static_cast<float>(zc_first)
        : 0.0f;
    EXPECT_GT(ratio, 0.10f);
}
