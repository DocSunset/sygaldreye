// Copyright 2025 Travis West
#pragma once
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include <memory>
#include <string_view>
#include "sphere_mesh.hpp"
#include "sygaldry_endpoints.hpp"

struct GlProgram;

struct SkyParams {
    // sun_elevation: -1.0 = midnight, 0.0 = horizon, +1.0 = overhead
    // When set, horizon/zenith colors are computed procedurally.
    float sun_elevation = 0.5f;
    // Optional manual override: set use_color_override=true to use these directly.
    bool            use_color_override = false;
    Eigen::Vector4f horizon_color{0.5f, 0.7f, 1.0f, 1.0f};
    Eigen::Vector4f zenith_color{0.1f, 0.2f, 0.6f, 1.0f};

    float radius = 500.0f;
    // Sun / moon direction and disc
    Eigen::Vector3f body_dir{0.0f, 1.0f, 0.0f};
    Eigen::Vector4f body_color{1.0f, 0.98f, 0.85f, 1.0f};
    float body_angular_radius = 0.009f;
    // Stars fade in when sun_elevation < 0
};

class SkyDome {
public:
    static consteval std::string_view name()          { return "sky_dome"; }
    static consteval std::string_view source_header() { return "components/sky_dome/sky_dome.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/sky_dome/sky_dome.cpp"; }

    struct endpoints {
        normalled_in<float, fp(-1.0f), fp(1.0f), fp(0.5f)> sun_elevation;
        normalled_in<float, fp(10.0f), fp(2000.0f), fp(500.0f)> radius;
    
        ::out<DrawFn> render;
        ::out<float> sun_elevation_out;
        ::out<float> sun_azimuth_out;
        ::out<Eigen::Vector3f> sun_dir;
        ::out<Eigen::Vector4f> sun_color;
    } endpoints;


    void operator()(double time_s);

    static SkyDome create(SkyParams const&);

    SkyDome() = default;
    ~SkyDome();
    SkyDome(const SkyDome&) = delete;
    SkyDome& operator=(const SkyDome&) = delete;
    SkyDome(SkyDome&&) noexcept;
    SkyDome& operator=(SkyDome&&) noexcept;

    void set_params(SkyParams const&);
    void draw(Eigen::Matrix4f const& vp) const;

private:
    void init_gl();
    SphereMesh dome_mesh_;
    std::unique_ptr<GlProgram> sky_prog_;
    GLint vp_loc_          = -1;
    GLint body_dir_loc_    = -1;
    GLint body_color_loc_  = -1;
    GLint body_cos_loc_    = -1;
    GLint sun_elev_loc_    = -1;
    GLint use_override_loc_= -1;
    GLint horizon_loc_     = -1;
    GLint zenith_loc_      = -1;

    SkyParams params_;
};
