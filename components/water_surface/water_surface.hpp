// Copyright 2025 Travis West
#pragma once
#include "tri_mesh.hpp"
#include "gl_program.hpp"
#include "light.hpp"
#include "material.hpp"
#include <Eigen/Core>
#include <memory>
#include <vector>

struct GerstnerWave {
    Eigen::Vector2f dir        = {1.0f, 0.0f};
    float           amplitude  = 0.3f;
    float           wavelength = 8.0f;
    float           steepness  = 0.5f; // 0=sine, 1=peaked trochoidal
    float           phase      = 0.0f; // initial phase offset (radians)
    // speed derived from physical dispersion: c = sqrt(g/k)
};

// 32 waves: harmonic series from 56m fundamental, A∝λ, random inharmonicity/phase/direction.
std::vector<GerstnerWave> make_default_waves();

struct WaterParams {
    int   grid_w    = 64;
    int   grid_h    = 64;
    float cell_size = 1.0f;
    std::vector<GerstnerWave> waves = make_default_waves();
    Eigen::Vector4f shallow_color  = {0.15f, 0.55f, 0.75f, 1.0f};
    Eigen::Vector4f deep_color     = {0.03f, 0.18f, 0.42f, 1.0f};
    float           foam_threshold = 0.55f;
    Light           sun            = {LightType::Directional,
                                      {-0.4f, -0.9f, -0.2f},
                                      {1.0f, 0.95f, 0.85f}, 1.2f};
    Material        material       = {{0.04f, 0.10f, 0.16f},
                                      {0.55f, 0.70f, 0.80f},
                                      {0.9f,  0.95f, 1.0f}, 80.0f};
};

class WaterSurface {
public:
    static WaterSurface create(WaterParams const&);
    void update(float time_s);
    void draw(Eigen::Matrix4f const& mvp,
              Eigen::Matrix4f const& model,
              Eigen::Vector3f const& view_pos) const;

private:
    WaterParams            params_;
    TriMeshData            data_;
    TriMesh                mesh_;
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_      = -1;
    GLint model_loc_    = -1;
    GLint view_pos_loc_ = -1;
    GLint sun_dir_loc_  = -1;
    GLint sun_col_loc_  = -1;
    GLint sun_int_loc_  = -1;
    GLint mat_amb_loc_  = -1;
    GLint mat_dif_loc_  = -1;
    GLint mat_spe_loc_  = -1;
    GLint mat_shi_loc_  = -1;
};
