// Copyright 2025 Travis West
#pragma once
#include "tri_mesh.hpp"
#include "gl_program.hpp"
#include <Eigen/Core>
#include <memory>

struct ChladniParams {
    int   grid_n   = 256;
    int   mode_m   = 3;
    int   mode_n   = 4;
    float omega    = 1.0f;
    Eigen::Vector4f node_color  = {0.95f, 0.90f, 0.75f, 1.0f}; // sand/cream
    Eigen::Vector4f anti_color  = {0.05f, 0.08f, 0.18f, 1.0f}; // dark blue
};

class Chladni {
public:
    static Chladni create(ChladniParams const&);
    void update(float time_s);
    void draw(Eigen::Matrix4f const& mvp) const;

private:
    ChladniParams params_;
    TriMeshData   data_;
    TriMesh       mesh_;
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_ = -1;
};
