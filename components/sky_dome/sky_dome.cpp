// Copyright 2025 Travis West
#include "sky_dome.hpp"
#include "sky_dome_shaders.hpp"
#include "gl_program.hpp"
#include "sphere_geometry.hpp"
#include "log.hpp"
#include <cmath>

#define TAG "sky_dome"
#define LOGE(...) LOG_E(TAG, __VA_ARGS__)
#define LOG(...)  LOG_I(TAG, __VA_ARGS__)

SkyDome SkyDome::create(SkyParams const& p) {
    SkyDome d;
    d.params_ = p;
    d.dome_mesh_ = SphereMesh::create(make_sphere(32, 16));

    auto sky = GlProgram::build(sky_dome_shaders::DOME_VERT, sky_dome_shaders::DOME_FRAG);
    if (!sky) { LOGE("sky shader build failed"); return d; }
    d.sky_prog_        = std::make_unique<GlProgram>(std::move(*sky));
    d.vp_loc_          = d.sky_prog_->uniform_location("uVP");
    d.body_dir_loc_    = d.sky_prog_->uniform_location("uBodyDir");
    d.body_color_loc_  = d.sky_prog_->uniform_location("uBodyColor");
    d.body_cos_loc_    = d.sky_prog_->uniform_location("uBodyCos");
    d.sun_elev_loc_    = d.sky_prog_->uniform_location("uSunElevation");
    d.use_override_loc_= d.sky_prog_->uniform_location("uUseOverride");
    d.horizon_loc_     = d.sky_prog_->uniform_location("uHorizon");
    d.zenith_loc_      = d.sky_prog_->uniform_location("uZenith");

    auto star = GlProgram::build(sky_dome_shaders::STAR_VERT, sky_dome_shaders::STAR_FRAG);
    if (!star) { LOGE("star shader build failed"); return d; }
    d.star_prog_       = std::make_unique<GlProgram>(std::move(*star));
    d.star_vp_loc_     = d.star_prog_->uniform_location("uVP");
    d.star_alpha_loc_  = d.star_prog_->uniform_location("uStarAlpha");
    d.star_radius_loc_ = d.star_prog_->uniform_location("uRadius");

    d.star_count_ = static_cast<GLsizei>(p.star_count);
    glGenVertexArrays(1, &d.star_vao_);
    return d;
}

void SkyDome::set_params(SkyParams const& p) { params_ = p; }

void SkyDome::operator()(double /*time_s*/) {
    // Sync params from input ports
    params_.sun_elevation = inputs.sun_elevation.value;
    params_.star_count    = static_cast<int>(inputs.star_count.value);
    params_.radius        = inputs.radius.value;
    outputs.render.value  = [this](const Eigen::Matrix4f& vp) { draw(vp); };
}

void SkyDome::draw(Eigen::Matrix4f const& vp) const {
    if (!sky_prog_) { LOGE("sky_prog_ is null, skipping draw"); return; }
    static bool once = false;
    if (!once) { once = true; LOG("sky_dome first draw: sun_elevation=%.2f", params_.sun_elevation); }
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    sky_prog_->use();
    GlProgram::uniform(vp_loc_, vp);
    glUniform3fv(body_dir_loc_,   1, params_.body_dir.normalized().eval().data());
    glUniform4fv(body_color_loc_, 1, params_.body_color.data());
    glUniform1f (body_cos_loc_,   std::cos(params_.body_angular_radius));
    glUniform1f (sun_elev_loc_,   params_.sun_elevation);
    glUniform1i (use_override_loc_, params_.use_color_override ? 1 : 0);
    glUniform4fv(horizon_loc_,    1, params_.horizon_color.data());
    glUniform4fv(zenith_loc_,     1, params_.zenith_color.data());
    dome_mesh_.draw();

    // Stars: fade in as sun dips below horizon
    float star_alpha = std::clamp(-params_.sun_elevation * 3.0f, 0.0f, 1.0f);
    if (star_count_ > 0 && star_alpha > 0.01f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        star_prog_->use();
        GlProgram::uniform(star_vp_loc_, vp);
        glUniform1f(star_alpha_loc_,  star_alpha);
        glUniform1f(star_radius_loc_, params_.radius);
        glBindVertexArray(star_vao_);
        glDrawArrays(GL_POINTS, 0, star_count_);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

SkyDome::~SkyDome() {
    if (star_vao_ != 0U) { glDeleteVertexArrays(1, &star_vao_); }
}

SkyDome::SkyDome(SkyDome&& src) noexcept
    : dome_mesh_(std::move(src.dome_mesh_)),
      sky_prog_(std::move(src.sky_prog_)),
      vp_loc_(src.vp_loc_),
      body_dir_loc_(src.body_dir_loc_),
      body_color_loc_(src.body_color_loc_),
      body_cos_loc_(src.body_cos_loc_),
      sun_elev_loc_(src.sun_elev_loc_),
      use_override_loc_(src.use_override_loc_),
      horizon_loc_(src.horizon_loc_),
      zenith_loc_(src.zenith_loc_),
      star_vao_(src.star_vao_),
      star_prog_(std::move(src.star_prog_)),
      star_vp_loc_(src.star_vp_loc_),
      star_alpha_loc_(src.star_alpha_loc_),
      star_radius_loc_(src.star_radius_loc_),
      star_count_(src.star_count_), params_(src.params_) {
    src.star_vao_ = 0U;
}

SkyDome& SkyDome::operator=(SkyDome&& src) noexcept {
    if (this != &src) {
        if (star_vao_ != 0U) { glDeleteVertexArrays(1, &star_vao_); }
        dome_mesh_        = std::move(src.dome_mesh_);
        sky_prog_         = std::move(src.sky_prog_);
        vp_loc_           = src.vp_loc_;
        body_dir_loc_     = src.body_dir_loc_;
        body_color_loc_   = src.body_color_loc_;
        body_cos_loc_     = src.body_cos_loc_;
        sun_elev_loc_     = src.sun_elev_loc_;
        use_override_loc_ = src.use_override_loc_;
        horizon_loc_      = src.horizon_loc_;
        zenith_loc_       = src.zenith_loc_;
        star_vao_         = src.star_vao_;  src.star_vao_ = 0U;
        star_prog_        = std::move(src.star_prog_);
        star_vp_loc_      = src.star_vp_loc_;
        star_alpha_loc_   = src.star_alpha_loc_;
        star_radius_loc_  = src.star_radius_loc_;
        star_count_       = src.star_count_;
        params_           = src.params_;
    }
    return *this;
}
