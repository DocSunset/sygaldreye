#include "scene.hpp"
#include <gtest/gtest.h>
#include <cmath>

TEST(SceneTest, UpdateProducesOneCube) {
    Scene s;
    s.update(0.0);
    EXPECT_EQ(s.cubes().size(), 1u);
}

TEST(SceneTest, ModelChangesOverTime) {
    Scene s;
    s.update(0.0);
    Eigen::Matrix4f m0 = s.cubes()[0].model;
    s.update(10.0);
    Eigen::Matrix4f m1 = s.cubes()[0].model;
    EXPECT_FALSE(m0.isApprox(m1, 1e-4f));
}

TEST(SceneTest, CubePlacement) {
    Scene s;
    s.update(0.0);
    auto m = s.cubes()[0].model;
    EXPECT_NEAR(m(0, 3), 0.0f, 1e-4f);
    EXPECT_NEAR(m(1, 3), 1.5f, 1e-4f);
    EXPECT_NEAR(m(2, 3), -5.0f, 1e-4f);
}

TEST(SceneTest, SetControllerPosesLeftOnly) {
    Scene s;
    s.update(0.0);
    XrPosef left_pose{};
    left_pose.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    left_pose.position = {0.0f, 1.0f, 0.0f};
    s.set_controller_poses(left_pose, std::nullopt);
    EXPECT_EQ(s.cubes().size(), 2u);  // 1 world + 1 hand
}

TEST(SceneTest, SetControllerPosesBothHands) {
    Scene s;
    s.update(0.0);
    XrPosef pose{};
    pose.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    pose.position = {0.0f, 0.0f, 0.0f};
    s.set_controller_poses(pose, pose);
    EXPECT_EQ(s.cubes().size(), 3u);  // 1 world + 2 hands
}

TEST(SceneTest, SetControllerPosesNoneValid) {
    Scene s;
    s.update(0.0);
    s.set_controller_poses(std::nullopt, std::nullopt);
    EXPECT_EQ(s.cubes().size(), 1u);  // only world cube
}

TEST(SceneTest, UpdateAfterSetControllerPreservesHands) {
    // After our fix (ticket 057), calling update() after set_controller_poses()
    // should NOT discard hand cubes — they are stored independently.
    Scene s;
    XrPosef pose{};
    pose.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    pose.position = {0.0f, 0.0f, 0.0f};
    s.set_controller_poses(pose, pose);
    s.update(0.0);  // This used to discard hand cubes; now it should not
    EXPECT_EQ(s.cubes().size(), 3u);  // 1 world + 2 hands still present
}
