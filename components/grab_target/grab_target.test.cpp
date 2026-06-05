#include "grab_target.hpp"
#include <gtest/gtest.h>

TEST(GrabTarget, DefaultState) {
    GrabTarget t;
    EXPECT_FALSE(t.grabbed);
    EXPECT_EQ(t.grabbing_hand, -1);
    EXPECT_FLOAT_EQ(t.radius, 0.05F);
}

TEST(GrabTarget, FieldAssignment) {
    GrabTarget t;
    t.grabbed = true;
    t.grabbing_hand = 0;
    t.position = {1.0F, 2.0F, 3.0F};
    EXPECT_TRUE(t.grabbed);
    EXPECT_EQ(t.grabbing_hand, 0);
}
