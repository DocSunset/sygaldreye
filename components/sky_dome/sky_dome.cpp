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
    d.init_gl();
    return d;
}

// Builds GL programs/meshes from params_. Called by create() and lazily on
// first tick: graph nodes are default-constructed, never via create().
void SkyDome::init_gl() {
    SkyDome& d = *this;
    SkyParams const& p = params_;
    d.dome_mesh_ = SphereMesh::create(make_sphere(32, 16));

    auto sky = GlProgram::build(sky_dome_shaders::DOME_VERT, sky_dome_shaders::DOME_FRAG);
    if (!sky) { LOGE("sky shader build failed"); return; }
    d.sky_prog_        = std::make_unique<GlProgram>(std::move(*sky));
    d.vp_loc_          = d.sky_prog_->uniform_location("uVP");
    d.body_dir_loc_    = d.sky_prog_->uniform_location("uBodyDir");
    d.body_color_loc_  = d.sky_prog_->uniform_location("uBodyColor");
    d.body_cos_loc_    = d.sky_prog_->uniform_location("uBodyCos");
    d.sun_elev_loc_    = d.sky_prog_->uniform_location("uSunElevation");
    d.use_override_loc_= d.sky_prog_->uniform_location("uUseOverride");
    d.horizon_loc_     = d.sky_prog_->uniform_location("uHorizon");
    d.zenith_loc_      = d.sky_prog_->uniform_location("uZenith");

    d.star_field_ = StarField::create(p.star_count, p.radius);
}

void SkyDome::set_params(SkyParams const& p) { params_ = p; }

void SkyDome::operator()(double /*time_s*/) {
    if (!sky_prog_) init_gl();
    // Sync params from input ports
    params_.sun_elevation = inputs.sun_elevation.value;
    params_.star_count    = static_cast<int>(inputs.star_count.value);
    params_.radius        = inputs.radius.value;
    star_field_.inputs.sun_elevation.value = inputs.sun_elevation.value;
    star_field_.inputs.star_count.value    = inputs.star_count.value;
    star_field_.inputs.radius.value        = inputs.radius.value;
    outputs.render.value  = [this](const Eigen::Matrix4f& vp) { draw(vp); };

    // Publish scalar outputs for downstream wiring
    const float el = inputs.sun_elevation.value;
    constexpr float az = 0.0f;  // no azimuth input yet; default to 0
    outputs.sun_elevation_out.value = el;
    outputs.sun_azimuth_out.value   = az;
    outputs.sun_dir.value = Eigen::Vector3f{
        std::cos(el) * std::sin(az),
        std::sin(el),
        std::cos(el) * std::cos(az)
    };
    outputs.sun_color.value = Eigen::Vector4f{1.f, 0.95f, 0.8f, 1.f};
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

    star_field_.draw(vp);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

SkyDome::~SkyDome() = default;

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
      star_field_(std::move(src.star_field_)),
      params_(src.params_) {}

SkyDome& SkyDome::operator=(SkyDome&& src) noexcept {
    if (this != &src) {
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
        star_field_       = std::move(src.star_field_);
        params_           = src.params_;
    }
    return *this;
}
