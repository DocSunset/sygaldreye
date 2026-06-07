// Copyright 2025 Travis West
#include "trigger_edge.hpp"
#include <gtest/gtest.h>

static TriggerEdge make(float trigger, float threshold = 0.5f) {
    TriggerEdge n;
    n.inputs.threshold.value = threshold;
    n.inputs.trigger.value   = trigger;
    n(0.0);
    return n;
}

TEST(TriggerEdge, PressOnRisingEdge) {
    TriggerEdge n;
    n.inputs.threshold.value = 0.5f;

    n.inputs.trigger.value = 0.f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.press.value, 0.f);

    n.inputs.trigger.value = 1.f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.press.value, 1.f);

    n.inputs.trigger.value = 1.f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.press.value, 0.f);
}

TEST(TriggerEdge, ReleaseOnFallingEdge) {
    TriggerEdge n;
    n.inputs.threshold.value = 0.5f;

    n.inputs.trigger.value = 1.f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.release.value, 0.f);

    n.inputs.trigger.value = 0.f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.release.value, 1.f);

    n.inputs.trigger.value = 0.f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.release.value, 0.f);
}

TEST(TriggerEdge, HeldWhileAboveThreshold) {
    TriggerEdge n;
    n.inputs.threshold.value = 0.5f;

    n.inputs.trigger.value = 0.f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.held.value, 0.f);

    n.inputs.trigger.value = 0.8f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.held.value, 1.f);

    n.inputs.trigger.value = 0.9f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.held.value, 1.f);

    n.inputs.trigger.value = 0.2f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.held.value, 0.f);
}

TEST(TriggerEdge, CustomThreshold) {
    TriggerEdge n;
    n.inputs.threshold.value = 0.8f;

    n.inputs.trigger.value = 0.7f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.press.value, 0.f);
    EXPECT_FLOAT_EQ(n.outputs.held.value,  0.f);

    n.inputs.trigger.value = 0.9f; n(0.0);
    EXPECT_FLOAT_EQ(n.outputs.press.value, 1.f);
    EXPECT_FLOAT_EQ(n.outputs.held.value,  1.f);
}
