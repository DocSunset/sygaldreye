// Copyright 2025 Travis West
#include "ptt_gate.hpp"
#include <gtest/gtest.h>

static float kAudio[4] = {0.1f, 0.2f, 0.3f, 0.4f};

// Inputs are v6: tests drive them the way edges do — src points at a value.
struct Rig {
    PttGate g;
    float open = 0.f, close = 0.f;
    AudioBuffer audio{};
    Rig() {
        g.endpoints.open.src     = &open;
        g.endpoints.close.src    = &close;
        g.endpoints.audio_in.src = &audio;
    }
    void tick(float o, float c) { open = o; close = c; g(0.0); }
};

TEST(PttGate, StartsClosedEmptyAudio) {
    PttGate g;
    g(0.0);
    EXPECT_EQ(g.endpoints.audio_out.value.frames, 0);
    EXPECT_EQ(g.endpoints.is_open.value, 0.f);
}

TEST(PttGate, OpenBangOpensGate) {
    Rig r;
    r.audio = AudioBuffer{kAudio, 4, 1, 16000};
    r.tick(1.f, 0.f);
    EXPECT_EQ(r.g.endpoints.is_open.value, 1.f);
    EXPECT_EQ(r.g.endpoints.opened.value, 1.f);
    EXPECT_EQ(r.g.endpoints.closed.value, 0.f);
    EXPECT_EQ(r.g.endpoints.audio_out.value.frames, 4);
    EXPECT_EQ(r.g.endpoints.audio_out.value.data, kAudio);
}

TEST(PttGate, OpenedNotLatchedNextTick) {
    Rig r;
    r.tick(1.f, 0.f);
    r.tick(0.f, 0.f);
    EXPECT_EQ(r.g.endpoints.opened.value, 0.f);
    EXPECT_EQ(r.g.endpoints.is_open.value, 1.f);
}

TEST(PttGate, CloseBangClosesGate) {
    Rig r;
    r.tick(1.f, 0.f);                 // open first
    r.audio = AudioBuffer{kAudio, 4, 1, 16000};
    r.tick(0.f, 1.f);
    EXPECT_EQ(r.g.endpoints.is_open.value, 0.f);
    EXPECT_EQ(r.g.endpoints.closed.value, 1.f);
    EXPECT_EQ(r.g.endpoints.audio_out.value.frames, 0);
}

TEST(PttGate, ClosedNotLatchedNextTick) {
    Rig r;
    r.tick(1.f, 0.f);                 // open
    r.tick(0.f, 1.f);                 // close
    r.tick(0.f, 0.f);
    EXPECT_EQ(r.g.endpoints.closed.value, 0.f);
    EXPECT_EQ(r.g.endpoints.is_open.value, 0.f);
}
