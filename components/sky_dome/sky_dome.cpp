// Copyright 2025 Travis West
#include "sky_dome.hpp"
#include "sky_dome_shaders.hpp"
#include "gl_program.hpp"
#include "sphere_geometry.hpp"
#include "log.hpp"
#include <cmath>
#include <vector>
#include <numbers>

#define TAG "sky_dome"
#define LOGE(...) LOG_E(TAG, __VA_ARGS__)

static std::vector<Eigen::Vector3f> gen_stars(int count, float radius) {
    std::vector<Eigen::Vector3f> pts;
    pts.reserve(static_cast<size_t>(count));
    uint32_t seed = 0xdeadbeef;
    auto rand01 = [&]() -> float {
        seed = seed * 1664525u + 1013904223u;
        return static_cast<float>(seed >> 8) / static_cast<float>(1u << 24);
    };
    for (int i = 0; i < count; ++i) {
        float theta = 2.0f * std::numbers::pi_v<float> * rand01();
        float phi   = std::acos(1.0f - 2.0f * rand01());
        float sp = std::sin(phi), cp = std::cos(phi);
        pts.push_back(radius * Eigen::Vector3f{std::cos(theta)*sp, cp, std::sin(theta)*sp});
    }
    return pts;
}

SkyDome SkyDome::create(SkyParams const& p) {
    SkyDome d;
    d.params_ = p;
    d.dome_mesh_ = SphereMesh::create(make_sphere(32, 16));

    auto sky = GlProgram::build(sky_dome_shaders::DOME_VERT, sky_dome_shaders::DOME_FRAG);
    if (!sky) { LOGE("sky shader build failed"); return d; }
    d.sky_prog_       = std::make_unique<GlProgram>(std::move(*sky));
    d.vp_loc_         = d.sky_prog_->uniform_location("uVP");
    d.horizon_loc_    = d.sky_prog_->uniform_location("uHorizon");
    d.zenith_loc_     = d.sky_prog_->uniform_location("uZenith");
    d.body_dir_loc_   = d.sky_prog_->uniform_location("uBodyDir");
    d.body_color_loc_ = d.sky_prog_->uniform_location("uBodyColor");
    d.body_cos_loc_   = d.sky_prog_->uniform_location("uBodyCos");

    auto star = GlProgram::build(sky_dome_shaders::STAR_VERT, sky_dome_shaders::STAR_FRAG);
    if (!star) { LOGE("star shader build failed"); return d; }
    d.star_prog_   = std::make_unique<GlProgram>(std::move(*star));
    d.star_vp_loc_ = d.star_prog_->uniform_location("uVP");

    auto pts = gen_stars(p.star_count, p.radius);
    d.star_count_ = static_cast<GLsizei>(pts.size());
    glGenVertexArrays(1, &d.star_vao_);
    glGenBuffers(1, &d.star_vbo_);
    glBindVertexArray(d.star_vao_);
    glBindBuffer(GL_ARRAY_BUFFER, d.star_vbo_);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(pts.size() * sizeof(Eigen::Vector3f)),
        pts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Eigen::Vector3f), nullptr);
    glBindVertexArray(0);
    return d;
}

void SkyDome::set_params(SkyParams const& p) { params_ = p; }

void SkyDome::draw(Eigen::Matrix4f const& vp) const {
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    sky_prog_->use();
    GlProgram::uniform(vp_loc_, vp);
    glUniform4fv(horizon_loc_,    1, params_.horizon_color.data());
    glUniform4fv(zenith_loc_,     1, params_.zenith_color.data());
    glUniform3fv(body_dir_loc_,   1, params_.body_dir.data());
    glUniform4fv(body_color_loc_, 1, params_.body_color.data());
    glUniform1f(body_cos_loc_, std::cos(params_.body_angular_radius));
    dome_mesh_.draw();

    if (params_.enable_stars && star_count_ > 0) {
        star_prog_->use();
        GlProgram::uniform(star_vp_loc_, vp);
        glBindVertexArray(star_vao_);
        glDrawArrays(GL_POINTS, 0, star_count_);
        glBindVertexArray(0);
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

SkyDome::~SkyDome() {
    if (star_vao_ != 0U) { glDeleteVertexArrays(1, &star_vao_); }
    if (star_vbo_ != 0U) { glDeleteBuffers(1, &star_vbo_); }
}

SkyDome::SkyDome(SkyDome&& src) noexcept
    : dome_mesh_(std::move(src.dome_mesh_)),
      sky_prog_(std::move(src.sky_prog_)),
      vp_loc_(src.vp_loc_), horizon_loc_(src.horizon_loc_),
      zenith_loc_(src.zenith_loc_), body_dir_loc_(src.body_dir_loc_),
      body_color_loc_(src.body_color_loc_), body_cos_loc_(src.body_cos_loc_),
      star_vao_(src.star_vao_), star_vbo_(src.star_vbo_),
      star_prog_(std::move(src.star_prog_)), star_vp_loc_(src.star_vp_loc_),
      star_count_(src.star_count_), params_(src.params_) {
    src.star_vao_ = 0U; src.star_vbo_ = 0U;
}

SkyDome& SkyDome::operator=(SkyDome&& src) noexcept {
    if (this != &src) {
        if (star_vao_ != 0U) { glDeleteVertexArrays(1, &star_vao_); }
        if (star_vbo_ != 0U) { glDeleteBuffers(1, &star_vbo_); }
        dome_mesh_       = std::move(src.dome_mesh_);
        sky_prog_        = std::move(src.sky_prog_);
        vp_loc_          = src.vp_loc_;
        horizon_loc_     = src.horizon_loc_;
        zenith_loc_      = src.zenith_loc_;
        body_dir_loc_    = src.body_dir_loc_;
        body_color_loc_  = src.body_color_loc_;
        body_cos_loc_    = src.body_cos_loc_;
        star_vao_        = src.star_vao_;  src.star_vao_ = 0U;
        star_vbo_        = src.star_vbo_;  src.star_vbo_ = 0U;
        star_prog_       = std::move(src.star_prog_);
        star_vp_loc_     = src.star_vp_loc_;
        star_count_      = src.star_count_;
        params_          = src.params_;
    }
    return *this;
}
