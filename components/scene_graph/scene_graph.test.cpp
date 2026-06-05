#include "scene_graph.hpp"
#include <gtest/gtest.h>

TEST(SceneGraph, IdentityDefault) {
    SceneGraph g;
    NodeId root = g.add_node();
    EXPECT_TRUE(g.world_transform(root).isApprox(Eigen::Matrix4f::Identity()));
}

TEST(SceneGraph, ParentChildComposition) {
    SceneGraph g;
    NodeId root  = g.add_node();
    NodeId child = g.add_node(root);
    NodeId leaf  = g.add_node(child);

    Eigen::Affine3f t_root  = Eigen::Affine3f(Eigen::Translation3f(1.f, 0.f, 0.f));
    Eigen::Affine3f t_child = Eigen::Affine3f(Eigen::Translation3f(0.f, 2.f, 0.f));
    Eigen::Affine3f t_leaf  = Eigen::Affine3f(Eigen::Translation3f(0.f, 0.f, 3.f));

    g.set_transform(root,  t_root);
    g.set_transform(child, t_child);
    g.set_transform(leaf,  t_leaf);

    Eigen::Matrix4f expected = (t_root * t_child * t_leaf).matrix();
    EXPECT_TRUE(g.world_transform(leaf).isApprox(expected));
}

TEST(SceneGraph, CacheInvalidationPropagates) {
    SceneGraph g;
    NodeId root  = g.add_node();
    NodeId child = g.add_node(root);

    Eigen::Affine3f t1 = Eigen::Affine3f(Eigen::Translation3f(1.f, 0.f, 0.f));
    Eigen::Affine3f t2 = Eigen::Affine3f(Eigen::Translation3f(5.f, 0.f, 0.f));

    g.set_transform(root, t1);
    (void)g.world_transform(child); // populate cache

    g.set_transform(root, t2);      // should invalidate child too
    Eigen::Matrix4f expected = t2.matrix();
    EXPECT_TRUE(g.world_transform(child).isApprox(expected));
}
