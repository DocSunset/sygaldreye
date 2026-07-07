// Copyright 2025 Travis West
#pragma once
#include <vector>
#include <cstdint>
#include <Eigen/Core>
#include "tri_mesh.hpp"
#include "billboard_quad.hpp"

struct BranchParams {
    float    length_scale  = 0.7f;
    float    radius_scale  = 0.6f;
    float    spread_angle  = 0.4f;
    int      branch_count  = 3;
    int      max_depth     = 5;
    float    angle_noise   = 0.3f;
    uint32_t seed          = 42;
};

struct TreeParams {
    BranchParams    branch;
    float           trunk_height         = 4.0f;
    float           trunk_radius         = 0.15f;
    int             cylinder_slices      = 5;
    bool            enable_leaves        = true;
    float           leaf_cluster_radius  = 0.5f;
    Eigen::Vector4f leaf_color           = {0.2f, 0.6f, 0.15f, 1.0f};
    Eigen::Vector4f bark_color           = {0.35f, 0.22f, 0.1f, 1.0f};
};

struct TreeMesh {
    TriMeshData                  branches;
    std::vector<BillboardInstance> leaves;
};

TreeMesh generate_tree(TreeParams const&);
