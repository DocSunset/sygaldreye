// Copyright 2026 Travis West
#include "ui_nodes.hpp"
#include <gtest/gtest.h>

// Interaction math only — draw closures are never invoked, so no GL needed.
// v6 inputs: params via fallback, ray/trigger driven through src pointers
// the way real edges deliver them.

struct SliderRig {
    UiSliderNode s;
    Eigen::Vector3f    ray_pos{9.f, 9.f, 9.f};
    Eigen::Quaternionf ray_rot = Eigen::Quaternionf::Identity();
    float trigger = 0.f;
    SliderRig() {
        s.endpoints.x.fallback = 0.f;   s.endpoints.y.fallback = 1.2f;
        s.endpoints.z.fallback = -0.8f; s.endpoints.width.fallback = 0.3f;
        s.endpoints.min.fallback = 0.f; s.endpoints.max.fallback = 10.f;
        s.endpoints.init.fallback = 5.f;
        s.endpoints.ray_pos.src = &ray_pos;
        s.endpoints.ray_rot.src = &ray_rot;
        s.endpoints.trigger.src = &trigger;
    }
};

TEST(UiSlider, InitValueBeforeAnyInteraction) {
    SliderRig r;                                  // ray nowhere near
    r.s(0.0);
    EXPECT_FLOAT_EQ(r.s.endpoints.value.value, 5.f);
}

TEST(UiSlider, TriggerDragSetsValueAndItPersists) {
    SliderRig r;
    r.ray_pos = {0.12f, 1.2f, 0.f};               // u = 0.9 on the track
    r.trigger = 1.f;
    r.s(0.0);
    EXPECT_NEAR(r.s.endpoints.value.value, 9.f, 1e-4f);

    r.trigger = 0.f;                              // release, move away
    r.ray_pos = {9.f, 9.f, 9.f};
    r.s(0.0);
    EXPECT_NEAR(r.s.endpoints.value.value, 9.f, 1e-4f);
}

TEST(UiSlider, HoverWithoutTriggerDoesNotChangeValue) {
    SliderRig r;
    r.ray_pos = {0.12f, 1.2f, 0.f};
    r.s(0.0);
    EXPECT_FLOAT_EQ(r.s.endpoints.value.value, 5.f);
}

TEST(UiButton, HoverAndPressTransitions) {
    UiButtonNode b;
    Eigen::Vector3f    ray_pos{9.f, 9.f, 9.f};
    Eigen::Quaternionf ray_rot = Eigen::Quaternionf::Identity();
    float trigger = 0.f;
    b.endpoints.x.fallback = 0.f; b.endpoints.y.fallback = 1.f;
    b.endpoints.z.fallback = -0.8f;
    b.endpoints.ray_pos.src = &ray_pos;
    b.endpoints.ray_rot.src = &ray_rot;
    b.endpoints.trigger.src = &trigger;

    b(0.0);
    EXPECT_FLOAT_EQ(b.endpoints.hover.value, 0.f);
    EXPECT_FLOAT_EQ(b.endpoints.pressed.value, 0.f);

    ray_pos = {0.f, 1.f, 0.f};                    // aimed at face
    b(0.0);
    EXPECT_FLOAT_EQ(b.endpoints.hover.value, 1.f);
    EXPECT_FLOAT_EQ(b.endpoints.pressed.value, 0.f);

    trigger = 1.f;
    b(0.0);
    EXPECT_FLOAT_EQ(b.endpoints.pressed.value, 1.f);
}

TEST(UiSlider, DegenerateQuaternionFallsBackToIdentity) {
    SliderRig r;
    r.ray_rot = Eigen::Quaternionf(0.f, 0.f, 0.f, 0.f);
    r.ray_pos = {0.12f, 1.2f, 0.f};
    r.trigger = 1.f;
    r.s(0.0);  // must not crash; identity fallback still hits the track
    EXPECT_NEAR(r.s.endpoints.value.value, 9.f, 1e-4f);
}
