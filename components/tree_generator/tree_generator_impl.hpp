// Copyright 2025 Travis West
#pragma once
#include "tree_generator.hpp"

void emit_cylinder(TriMeshData& mesh,
                   Eigen::Vector3f const& base,
                   Eigen::Vector3f const& tip,
                   float r_bot, float r_top,
                   int slices,
                   Eigen::Vector4f const& color);

void recurse(TriMeshData& branches,
             std::vector<BillboardInstance>& leaves,
             Eigen::Vector3f const& base,
             Eigen::Vector3f const& dir,
             float length, float radius,
             int depth,
             TreeParams const& tp,
             BranchParams bp);
