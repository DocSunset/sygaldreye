#include "grab_target.hpp"
#include <gtest/gtest.h>

TEST(GrabTarget, DefaultState) {
    GrabTarget target;
    EXPECT_FALSE(target.grabbed);
    EXPECT_EQ(target.grabbing_hand, -1);
    EXPECT_FLOAT_EQ(target.radius, 0.05F);
}

TEST(GrabTarget, FieldAssignment) {
    GrabTarget target;
    target.grabbed = true;
    target.grabbing_hand = 0;
    target.position = {1.0F, 2.0F, 3.0F};
    EXPECT_TRUE(target.grabbed);
    EXPECT_EQ(target.grabbing_hand, 0);
}
