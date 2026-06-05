// Copyright 2024 Travis West
#include "synth_core.hpp"
#include <gtest/gtest.h>

static constexpr float k2Pi = 6.28318530f;

TEST(SynthCore, OscillatorsBounded) {
    int steps = 1000;
    for (int i = 0; i < steps; ++i) {
        float phase = k2Pi * static_cast<float>(i) / static_cast<float>(steps);
        EXPECT_GE(synth::sine(phase),     -1.0f);
        EXPECT_LE(synth::sine(phase),      1.0f);
        EXPECT_GE(synth::square(phase),   -1.0f);
        EXPECT_LE(synth::square(phase),    1.0f);
        EXPECT_GE(synth::sawtooth(phase), -1.0f);
        EXPECT_LE(synth::sawtooth(phase),  1.0f);
        EXPECT_GE(synth::triangle(phase), -1.0f);
        EXPECT_LE(synth::triangle(phase),  1.0f);
    }
}

TEST(SynthCore, AdsrStagesForward) {
    synth::Adsr adsr;
    adsr.attack_s      = 100.0f / 48000.0f;
    adsr.decay_s       = 100.0f / 48000.0f;
    adsr.sustain_level = 0.5f;
    adsr.release_s     = 100.0f / 48000.0f;
    adsr.sample_rate   = 48000.0f;

    adsr.gate_on();
    EXPECT_EQ(adsr.stage, synth::Adsr::Stage::Attack);

    for (int i = 0; i < 101; ++i) { adsr.tick(); }
    EXPECT_EQ(adsr.stage, synth::Adsr::Stage::Decay);

    for (int i = 0; i < 101; ++i) { adsr.tick(); }
    EXPECT_EQ(adsr.stage, synth::Adsr::Stage::Sustain);

    adsr.gate_off();
    EXPECT_EQ(adsr.stage, synth::Adsr::Stage::Release);

    for (int i = 0; i < 101; ++i) { adsr.tick(); }
    EXPECT_EQ(adsr.stage, synth::Adsr::Stage::Done);
    EXPECT_FLOAT_EQ(adsr.tick(), 0.0f);
}

TEST(SynthCore, WhiteNoiseBoundedAndVarying) {
    uint32_t state   = 12345u;
    float    prev    = synth::white_noise(state);
    bool     varying = false;
    for (int i = 0; i < 1000; ++i) {
        float v = synth::white_noise(state);
        EXPECT_GE(v, -1.0f);
        EXPECT_LE(v,  1.0f);
        if (v != prev) { varying = true; }
        prev = v;
    }
    EXPECT_TRUE(varying);
}
