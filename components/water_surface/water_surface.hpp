// Copyright 2025 Travis West
#pragma once
#include "tri_mesh.hpp"
#include "gl_program.hpp"
#include <Eigen/Core>
#include <memory>

struct WaterParams {
    int   grid_w      = 32;
    int   grid_h      = 32;
    float cell_size   = 1.0f;
    float wave_height = 0.08f;
    float wave_speed  = 0.6f;
    Eigen::Vector4f shallow_color = {0.1f, 0.5f, 0.7f, 0.85f};
    Eigen::Vector4f deep_color    = {0.03f, 0.2f, 0.45f, 0.9f};
};

class WaterSurface {
public:
    static WaterSurface create(WaterParams const&);
    void update(float time_s);
    void draw(Eigen::Matrix4f const& mvp) const;
    TriMeshData const& mesh_data() const { return data_; }

private:
    WaterParams             params_;
    TriMeshData             data_;
    TriMesh                 mesh_;
    std::unique_ptr<GlProgram> prog_;
    GLint                   mvp_loc_ = -1;
};
