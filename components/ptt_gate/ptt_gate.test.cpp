// Copyright 2025 Travis West
#include "ptt_gate.hpp"
#include <gtest/gtest.h>

static float kAudio[4] = {0.1f, 0.2f, 0.3f, 0.4f};

static PttGate make_gate(float open_bang = 0.f, float close_bang = 0.f,
                         const float* data = nullptr, int frames = 0) {
    PttGate g;
    g.inputs.open.value       = open_bang;
    g.inputs.close.value      = close_bang;
    g.inputs.audio_in.value   = AudioBuffer{data, frames, 1, 16000};
    g(0.0);
    return g;
}

TEST(PttGate, StartsClosedEmptyAudio) {
    PttGate g;
    g(0.0);
    EXPECT_EQ(g.outputs.audio_out.value.frames, 0);
    EXPECT_EQ(g.outputs.is_open.value, 0.f);
}

TEST(PttGate, OpenBangOpensGate) {
    auto g = make_gate(1.f, 0.f, kAudio, 4);
    EXPECT_EQ(g.outputs.is_open.value, 1.f);
    EXPECT_EQ(g.outputs.opened.value, 1.f);
    EXPECT_EQ(g.outputs.closed.value, 0.f);
    EXPECT_EQ(g.outputs.audio_out.value.frames, 4);
    EXPECT_EQ(g.outputs.audio_out.value.data, kAudio);
}

TEST(PttGate, OpenedNotLatchedNextTick) {
    auto g = make_gate(1.f);
    g.inputs.open.value  = 0.f;
    g.inputs.close.value = 0.f;
    g(0.0);
    EXPECT_EQ(g.outputs.opened.value, 0.f);
    EXPECT_EQ(g.outputs.is_open.value, 1.f);
}

TEST(PttGate, CloseBangClosesGate) {
    auto g = make_gate(1.f);          // open first
    g.inputs.open.value  = 0.f;
    g.inputs.close.value = 1.f;
    g.inputs.audio_in.value = AudioBuffer{kAudio, 4, 1, 16000};
    g(0.0);
    EXPECT_EQ(g.outputs.is_open.value, 0.f);
    EXPECT_EQ(g.outputs.closed.value, 1.f);
    EXPECT_EQ(g.outputs.audio_out.value.frames, 0);
}

TEST(PttGate, ClosedNotLatchedNextTick) {
    auto g = make_gate(1.f);          // open
    g.inputs.open.value  = 0.f;
    g.inputs.close.value = 1.f;
    g(0.0);                           // close
    g.inputs.close.value = 0.f;
    g(0.0);
    EXPECT_EQ(g.outputs.closed.value, 0.f);
    EXPECT_EQ(g.outputs.is_open.value, 0.f);
}
