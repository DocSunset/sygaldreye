// Copyright 2025 Travis West
#pragma once
#include <memory>
#include <string_view>
#include <vector>

#include <Eigen/Core>

#include "render_payloads.hpp"  // MeshPtr, Surface, Mesh, Shader, Uniform
#include "sygaldry_endpoints.hpp"

struct GerstnerWave {
    Eigen::Vector2f dir        = {1.0f, 0.0f};
    float           amplitude  = 0.3f;
    float           wavelength = 8.0f;
    float           steepness  = 0.5f;  // 0=sine, 1=peaked trochoidal
    float           phase      = 0.0f;  // initial phase offset (radians)
};

// 32 waves: harmonic series from 56m fundamental, A∝λ, random inharmonicity.
std::vector<GerstnerWave> make_waves(float base_wavelength, float amp_coeff, float base_steepness);
std::vector<GerstnerWave> make_default_waves();

// water_surface: a shader-specific node. A static flat grid (Mesh) + a Surface
// whose vertex shader displaces it with Gerstner waves animated by uTime
// (injected by render_region). Wave params ride as indexed uniform names
// (uWaveA[i]/uWaveB[i]); material/colors are baked. Sun is wireable. GL left
// this node — geometry is static; only the shader animates.
class WaterSurface {
   public:
    static consteval std::string_view name() { return "water_surface"; }
    static consteval std::string_view source_header() {
        return "components/water_surface/water_surface.hpp";
    }
    static consteval std::string_view source_cpp() {
        return "components/water_surface/water_surface.cpp";
    }

    struct endpoints {
        normalled_in<float, fp(0.1f), fp(5.0f), fp(1.0f)> cell_size;
        normalled_in<float, fp(0.0f), fp(1.0f), fp(0.55f)> foam_threshold;
        normalled_in<float, fp(0.0f), fp(5.0f), fp(1.2f)> sun_intensity;
        normalled_in<float, fp(8.0f), fp(150.0f), fp(56.0f)> wavelength;
        normalled_in<float, fp(0.0f), fp(1.0f), fp(0.6f)> choppiness;
        normalled_in<float, fp(0.0f), fp(0.05f), fp(0.016f)> amplitude;
        ::in<Eigen::Vector3f> sun_dir;
        ::in<Eigen::Vector3f> sun_color;
        ::out<Surface>        surface;
        ::out<Mesh>           mesh;
    } endpoints;

    void operator()(double time_s);

   private:
    void rebuild_waves();

    MeshPtr              grid_;
    Shader               shader_;
    std::vector<Uniform> wave_uniforms_;  // 64 entries (uWaveA/B[0..31])
    float                cell_      = 1.0f;
    float                spec_wl_   = -1.f;
    float                spec_steep_ = -1.f;
    float                spec_amp_  = -1.f;
    float                max_amp_   = 0.f;
};
