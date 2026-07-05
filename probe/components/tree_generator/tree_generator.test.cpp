// Copyright 2025 Travis West
#include "tree_generator.hpp"
#include <gtest/gtest.h>
#include <cmath>

static TreeParams make_params(int max_depth, int branch_count) {
    TreeParams tp;
    tp.branch.max_depth    = max_depth;
    tp.branch.branch_count = branch_count;
    tp.enable_leaves       = true;
    return tp;
}

TEST(tree_generator, indices_nonzero) {
    auto mesh = generate_tree(make_params(2, 2));
    EXPECT_GT(mesh.branches.indices.size(), 0u);
}

TEST(tree_generator, leaf_count_matches_tips) {
    // depth=2, branch_count=2: 2^2 = 4 tips
    auto mesh = generate_tree(make_params(2, 2));
    EXPECT_EQ(mesh.leaves.size(), 4u);
}

TEST(tree_generator, normals_unit_length) {
    auto mesh = generate_tree(make_params(3, 3));
    for (auto const& v : mesh.branches.vertices) {
        float len = v.normal.norm();
        EXPECT_NEAR(len, 1.0f, 1e-5f);
    }
}
