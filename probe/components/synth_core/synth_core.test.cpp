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

// ── kernels (planning/kernel_extraction.md) ─────────────────────────────────
#include "kernels.hpp"

TEST(Kernels, PinkNoiseBoundedAndColored) {
    synth::PinkNoise k;
    float peak_w = 0.f;
    for (int i = 0; i < 48000; ++i)
        peak_w = std::max(peak_w, std::abs(k.tick(0.f)));
    EXPECT_GT(peak_w, 0.5f);
    EXPECT_LE(peak_w, 1.0f);
}

TEST(Kernels, DelayLineEchoesAtDelay) {
    synth::DelayLine d;
    d.prepare(100);
    EXPECT_FLOAT_EQ(d.tick(1.f, 10, 0.f), 0.f);   // impulse in
    for (int i = 1; i < 10; ++i) EXPECT_FLOAT_EQ(d.tick(0.f, 10, 0.f), 0.f);
    EXPECT_FLOAT_EQ(d.tick(0.f, 10, 0.f), 1.f);   // echo exactly 10 later
}

TEST(Kernels, SampleHoldHoldsBetweenClocks) {
    synth::SampleHold k;
    EXPECT_FLOAT_EQ(k.tick(3.f, true), 3.f);
    EXPECT_FLOAT_EQ(k.tick(9.f, false), 3.f);
    EXPECT_FLOAT_EQ(k.tick(9.f, true), 9.f);
}

TEST(Kernels, SlewLimitsStep) {
    synth::Slew k;
    EXPECT_FLOAT_EQ(k.tick(10.f, 1.f), 1.f);
    EXPECT_FLOAT_EQ(k.tick(10.f, 1.f), 2.f);
    EXPECT_FLOAT_EQ(k.tick(0.f, 100.f), 0.f);
}

TEST(Kernels, GrainVoiceDecaysToSilence) {
    synth::GrainVoice v;
    uint32_t rng = 1;
    v.strike(440.f);
    float peak = 0.f;   // sine starts at 0; peak over the first cycle instead
    for (int i = 0; i < 200; ++i)
        peak = std::max(peak, std::abs(v.tick(0.f, 0.99f, rng, 48000.f)));
    for (int i = 0; i < 2000; ++i) v.tick(0.f, 0.99f, rng, 48000.f);
    EXPECT_GT(peak, 0.1f);
    EXPECT_FLOAT_EQ(v.tick(0.f, 0.99f, rng, 48000.f), 0.f);  // env floor
}
