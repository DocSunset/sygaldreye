// Copyright 2026 Travis West
#include "ui_nodes.hpp"
#include <gtest/gtest.h>

// Interaction math only — draw closures are never invoked, so no GL needed.

static UiSliderNode make_slider() {
    UiSliderNode s;
    s.inputs.x.value = 0.f;   s.inputs.y.value = 1.2f; s.inputs.z.value = -0.8f;
    s.inputs.width.value = 0.3f;
    s.inputs.min.value = 0.f; s.inputs.max.value = 10.f; s.inputs.init.value = 5.f;
    s.inputs.ray_rot.value = Eigen::Quaternionf::Identity();
    return s;
}

TEST(UiSlider, InitValueBeforeAnyInteraction) {
    auto s = make_slider();
    s.inputs.ray_pos.value = {9.f, 9.f, 9.f};  // ray nowhere near
    s(0.0);
    EXPECT_FLOAT_EQ(s.outputs.value.value, 5.f);
}

TEST(UiSlider, TriggerDragSetsValueAndItPersists) {
    auto s = make_slider();
    s.inputs.ray_pos.value = {0.12f, 1.2f, 0.f};  // u = 0.9 on the track
    s.inputs.trigger.value = 1.f;
    s(0.0);
    EXPECT_NEAR(s.outputs.value.value, 9.f, 1e-4f);

    s.inputs.trigger.value = 0.f;                 // release, move away
    s.inputs.ray_pos.value = {9.f, 9.f, 9.f};
    s(0.0);
    EXPECT_NEAR(s.outputs.value.value, 9.f, 1e-4f);
}

TEST(UiSlider, HoverWithoutTriggerDoesNotChangeValue) {
    auto s = make_slider();
    s.inputs.ray_pos.value = {0.12f, 1.2f, 0.f};
    s.inputs.trigger.value = 0.f;
    s(0.0);
    EXPECT_FLOAT_EQ(s.outputs.value.value, 5.f);
}

TEST(UiButton, HoverAndPressTransitions) {
    UiButtonNode b;
    b.inputs.x.value = 0.f; b.inputs.y.value = 1.f; b.inputs.z.value = -0.8f;
    b.inputs.ray_rot.value = Eigen::Quaternionf::Identity();

    b.inputs.ray_pos.value = {9.f, 9.f, 9.f};
    b(0.0);
    EXPECT_FLOAT_EQ(b.outputs.hover.value, 0.f);
    EXPECT_FLOAT_EQ(b.outputs.pressed.value, 0.f);

    b.inputs.ray_pos.value = {0.f, 1.f, 0.f};     // aimed at face
    b(0.0);
    EXPECT_FLOAT_EQ(b.outputs.hover.value, 1.f);
    EXPECT_FLOAT_EQ(b.outputs.pressed.value, 0.f);

    b.inputs.trigger.value = 1.f;
    b(0.0);
    EXPECT_FLOAT_EQ(b.outputs.pressed.value, 1.f);
}

TEST(UiSlider, DegenerateQuaternionFallsBackToIdentity) {
    auto s = make_slider();
    s.inputs.ray_rot.value = Eigen::Quaternionf(0.f, 0.f, 0.f, 0.f);
    s.inputs.ray_pos.value = {0.12f, 1.2f, 0.f};
    s.inputs.trigger.value = 1.f;
    s(0.0);  // must not crash; identity fallback still hits the track
    EXPECT_NEAR(s.outputs.value.value, 9.f, 1e-4f);
}
