// Copyright 2025 Travis West
#include "trigger_edge.hpp"
#include <gtest/gtest.h>

// Inputs are v6: tests drive them the way edges do — src points at a value.
struct Rig {
    TriggerEdge n;
    float trigger = 0.f;
    Rig(float threshold = 0.5f) {
        n.endpoints.trigger.src = &trigger;
        n.endpoints.threshold.fallback = threshold;
    }
    void tick(float t) { trigger = t; n(0.0); }
};

TEST(TriggerEdge, PressOnRisingEdge) {
    Rig r;
    r.tick(0.f);
    EXPECT_FLOAT_EQ(r.n.endpoints.press.value, 0.f);
    r.tick(1.f);
    EXPECT_FLOAT_EQ(r.n.endpoints.press.value, 1.f);
    r.tick(1.f);
    EXPECT_FLOAT_EQ(r.n.endpoints.press.value, 0.f);
}

TEST(TriggerEdge, ReleaseOnFallingEdge) {
    Rig r;
    r.tick(1.f);
    EXPECT_FLOAT_EQ(r.n.endpoints.release.value, 0.f);
    r.tick(0.f);
    EXPECT_FLOAT_EQ(r.n.endpoints.release.value, 1.f);
    r.tick(0.f);
    EXPECT_FLOAT_EQ(r.n.endpoints.release.value, 0.f);
}

TEST(TriggerEdge, HeldWhileAboveThreshold) {
    Rig r;
    r.tick(0.f);
    EXPECT_FLOAT_EQ(r.n.endpoints.held.value, 0.f);
    r.tick(0.8f);
    EXPECT_FLOAT_EQ(r.n.endpoints.held.value, 1.f);
    r.tick(0.9f);
    EXPECT_FLOAT_EQ(r.n.endpoints.held.value, 1.f);
    r.tick(0.2f);
    EXPECT_FLOAT_EQ(r.n.endpoints.held.value, 0.f);
}

TEST(TriggerEdge, CustomThreshold) {
    Rig r(0.8f);
    r.tick(0.7f);
    EXPECT_FLOAT_EQ(r.n.endpoints.press.value, 0.f);
    EXPECT_FLOAT_EQ(r.n.endpoints.held.value,  0.f);
    r.tick(0.9f);
    EXPECT_FLOAT_EQ(r.n.endpoints.press.value, 1.f);
    EXPECT_FLOAT_EQ(r.n.endpoints.held.value,  1.f);
}
