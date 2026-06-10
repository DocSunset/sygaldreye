// Copyright 2025 Travis West
#pragma once
#include "gl_program.hpp"
#include <string_view>
#include "light.hpp"
#include "material.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
#include <GLES3/gl3.h>
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
std::vector<GerstnerWave> make_waves(float base_wavelength, float amp_coeff,
                                     float base_steepness);
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
    static consteval std::string_view name()          { return "water_surface"; }
    static consteval std::string_view source_header() { return "components/water_surface/water_surface.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/water_surface/water_surface.cpp"; }

    struct inputs {
        slider<"cell size",      "", float, 0.1, 5.0,  1.0>  cell_size;
        slider<"foam threshold", "", float, 0.0, 1.0,  0.55> foam_threshold;
        slider<"sun intensity",  "", float, 0.0, 5.0,  1.2>  sun_intensity;
        slider<"wavelength",     "", float, 8.0,  150.0, 56.0> wavelength;
        slider<"choppiness",     "", float, 0.0,  1.0,   0.6>  choppiness;
        slider<"amplitude",      "", float, 0.0,  0.05,  0.016> amplitude;
    } inputs;

    struct outputs {
        port<"render", DrawFn> render;
    } outputs;

    void operator()(double time_s);

    static WaterSurface create(WaterParams const&);

    WaterSurface() = default;
    ~WaterSurface();
    WaterSurface(const WaterSurface&) = delete;
    WaterSurface& operator=(const WaterSurface&) = delete;
    WaterSurface(WaterSurface&&) noexcept;
    WaterSurface& operator=(WaterSurface&&) noexcept;

    void set_sun(Light const& l) { params_.sun = l; }
    void update(float time_s) {
        time_s_                = time_s;
        params_.cell_size      = inputs.cell_size.value;
        params_.foam_threshold = inputs.foam_threshold.value;
        params_.sun.intensity  = inputs.sun_intensity.value;
    }
    void draw(Eigen::Matrix4f const& mvp,
              Eigen::Matrix4f const& model,
              Eigen::Vector3f const& view_pos) const;

private:
    void init_gl();
    float spec_wl_ = 56.f, spec_steep_ = 0.6f, spec_amp_ = 0.016f;
    void upload_wave_params();

    WaterParams                params_;
    float                      time_s_          = 0.0f;
    float                      max_amp_         = 0.0f;
    GLuint                     vao_             = 0;
    GLuint                     vbo_             = 0;
    GLuint                     ebo_             = 0;
    GLsizei                    index_count_     = 0;
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_         = -1;
    GLint model_loc_       = -1;
    GLint view_pos_loc_    = -1;
    GLint time_loc_        = -1;
    GLint sun_dir_loc_     = -1;
    GLint sun_col_loc_     = -1;
    GLint sun_int_loc_     = -1;
    GLint mat_amb_loc_     = -1;
    GLint mat_dif_loc_     = -1;
    GLint mat_spe_loc_     = -1;
    GLint mat_shi_loc_     = -1;
    GLint shallow_loc_     = -1;
    GLint deep_loc_        = -1;
    GLint foam_thresh_loc_ = -1;
    GLint max_amp_loc_     = -1;
    GLint wave_a_loc_      = -1;
    GLint wave_b_loc_      = -1;
};
