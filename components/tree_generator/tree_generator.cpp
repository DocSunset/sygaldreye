// Copyright 2025 Travis West
#include "tree_generator.hpp"
#include "tree_generator_impl.hpp"

TreeMesh generate_tree(TreeParams const& tp) {
    TreeMesh result;
    recurse(result.branches, result.leaves,
            {0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            tp.trunk_height, tp.trunk_radius,
            0, tp, tp.branch);
    return result;
}
