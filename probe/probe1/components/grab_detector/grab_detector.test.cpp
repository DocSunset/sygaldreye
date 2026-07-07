// Copyright 2024 Travis West
#include "grab_detector.hpp"
#include "grab_target.hpp"
#include <array>
#include <gtest/gtest.h>

TEST(GrabDetector, GrabAcquisition) {
    GrabTarget tgt;
    tgt.position = Eigen::Vector3f{0.0F, 0.0F, 0.0F};
    tgt.radius   = 0.1F;

    HandState hand;
    hand.position     = Eigen::Vector3f{0.05F, 0.0F, 0.0F};
    hand.valid        = true;
    hand.grip_pressed = true;

    std::array<HandState, 1>  hands{hand};
    std::array<GrabTarget, 1> targets{tgt};
    update_grabs(hands, targets);

    EXPECT_TRUE(targets[0].grabbed);
    EXPECT_EQ(targets[0].grabbing_hand, 0);
    Eigen::Vector3f expected_offset = tgt.position - hand.position;
    EXPECT_TRUE(targets[0].grab_offset.isApprox(expected_offset));
}

TEST(GrabDetector, NoGrabWhenOutOfRange) {
    GrabTarget tgt;
    tgt.position = Eigen::Vector3f{0.0F, 0.0F, 0.0F};
    tgt.radius   = 0.05F;

    HandState hand;
    hand.position     = Eigen::Vector3f{0.1F, 0.0F, 0.0F};
    hand.valid        = true;
    hand.grip_pressed = true;

    std::array<HandState, 1>  hands{hand};
    std::array<GrabTarget, 1> targets{tgt};
    update_grabs(hands, targets);

    EXPECT_FALSE(targets[0].grabbed);
}

TEST(GrabDetector, Release) {
    GrabTarget tgt;
    tgt.position      = Eigen::Vector3f{0.0F, 0.0F, 0.0F};
    tgt.radius        = 0.1F;
    tgt.grabbed       = true;
    tgt.grabbing_hand = 0;

    HandState hand;
    hand.position     = Eigen::Vector3f{0.05F, 0.0F, 0.0F};
    hand.valid        = true;
    hand.grip_pressed = false;

    std::array<HandState, 1>  hands{hand};
    std::array<GrabTarget, 1> targets{tgt};
    update_grabs(hands, targets);

    EXPECT_FALSE(targets[0].grabbed);
    EXPECT_EQ(targets[0].grabbing_hand, -1);
}

TEST(GrabDetector, NoDoubleGrab) {
    GrabTarget tgt_a;
    tgt_a.position = Eigen::Vector3f{0.02F, 0.0F, 0.0F};
    tgt_a.radius   = 0.1F;

    GrabTarget tgt_b;
    tgt_b.position = Eigen::Vector3f{-0.02F, 0.0F, 0.0F};
    tgt_b.radius   = 0.1F;

    HandState hand;
    hand.position     = Eigen::Vector3f{0.0F, 0.0F, 0.0F};
    hand.valid        = true;
    hand.grip_pressed = true;

    std::array<HandState, 1>  hands{hand};
    std::array<GrabTarget, 2> targets{tgt_a, tgt_b};
    update_grabs(hands, targets);

    int grab_count = (targets[0].grabbed ? 1 : 0) + (targets[1].grabbed ? 1 : 0);
    EXPECT_EQ(grab_count, 1);
}

TEST(GrabDetector, InvalidHandDoesNotGrab) {
    GrabTarget tgt;
    tgt.position = Eigen::Vector3f{0.0F, 0.0F, 0.0F};
    tgt.radius   = 0.1F;

    HandState hand;
    hand.position     = Eigen::Vector3f{0.05F, 0.0F, 0.0F};
    hand.valid        = false;
    hand.grip_pressed = true;

    std::array<HandState, 1>  hands{hand};
    std::array<GrabTarget, 1> targets{tgt};
    update_grabs(hands, targets);

    EXPECT_FALSE(targets[0].grabbed);
}
