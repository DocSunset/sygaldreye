// Copyright 2025 Travis West
#pragma once
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include <memory>
#include "sphere_mesh.hpp"

struct GlProgram;

struct SkyParams {
    Eigen::Vector4f horizon_color{0.5f, 0.7f, 1.0f, 1.0f};
    Eigen::Vector4f zenith_color{0.1f, 0.2f, 0.6f, 1.0f};
    float radius = 500.0f;
    Eigen::Vector3f body_dir{0.0f, 1.0f, 0.0f};
    Eigen::Vector4f body_color{1.0f, 1.0f, 0.9f, 0.0f};
    float body_angular_radius = 0.009f;
    int star_count = 2000;
    bool enable_stars = false;
};

class SkyDome {
public:
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
    SphereMesh dome_mesh_;
    std::unique_ptr<GlProgram> sky_prog_;
    GLint vp_loc_      = -1;
    GLint horizon_loc_ = -1;
    GLint zenith_loc_  = -1;
    GLint body_dir_loc_   = -1;
    GLint body_color_loc_ = -1;
    GLint body_cos_loc_   = -1;

    GLuint star_vao_   = 0;
    GLuint star_vbo_   = 0;
    std::unique_ptr<GlProgram> star_prog_;
    GLint star_vp_loc_ = -1;
    GLsizei star_count_ = 0;

    SkyParams params_;
};
