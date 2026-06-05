// Copyright 2025 Travis West
#pragma once
#include "tri_mesh.hpp"
#include "gl_program.hpp"
#include <Eigen/Core>
#include <memory>
#include <vector>

struct RDParams {
    int   grid_w = 256, grid_h = 256;
    float Du = 0.16f, Dv = 0.08f, F = 0.060f, k = 0.062f;
    float dt = 1.0f;
    int   steps_per_frame = 8;
    Eigen::Vector4f color_a = {0.05f, 0.15f, 0.35f, 1.0f}; // deep blue
    Eigen::Vector4f color_b = {0.95f, 0.90f, 0.70f, 1.0f}; // cream
};

class ReactionDiffusion {
public:
    static ReactionDiffusion create(RDParams const&);
    void update();
    void draw(Eigen::Matrix4f const& mvp) const;

private:
    RDParams                   params_;
    std::vector<float>         u_, v_, u_next_, v_next_;
    TriMeshData                data_;
    TriMesh                    mesh_;
    std::unique_ptr<GlProgram> prog_;
    GLint                      mvp_loc_ = -1;
};
