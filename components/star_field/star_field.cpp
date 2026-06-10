// Copyright 2025 Travis West
#include "star_field.hpp"
#include "star_field_shaders.hpp"
#include "gl_program.hpp"
#include "log.hpp"
#include <algorithm>
#include <cmath>

#define TAG "star_field"
#define LOGE(...) LOG_E(TAG, __VA_ARGS__)

StarField StarField::create(int star_count, float radius) {
    StarField sf;
    auto prog = GlProgram::build(star_field_shaders::STAR_VERT, star_field_shaders::STAR_FRAG);
    if (!prog) { LOGE("star shader build failed"); return sf; }
    sf.prog_        = std::make_unique<GlProgram>(std::move(*prog));
    sf.vp_loc_      = sf.prog_->uniform_location("uVP");
    sf.alpha_loc_   = sf.prog_->uniform_location("uStarAlpha");
    sf.radius_loc_  = sf.prog_->uniform_location("uRadius");
    sf.star_count_  = static_cast<GLsizei>(star_count);
    sf.radius_      = radius;
    glGenVertexArrays(1, &sf.vao_);
    return sf;
}

void StarField::operator()(double /*time_s*/) {
    if (!prog_) {  // constructed off render thread (graph parse): lazy init
        auto saved = inputs;
        *this = create(int(inputs.star_count.value), inputs.radius.value);
        inputs = saved;
    }
    sun_elev_   = inputs.sun_elevation.value;
    star_count_ = static_cast<GLsizei>(inputs.star_count.value);
    radius_     = inputs.radius.value;
    outputs.render.value = [this](const Eigen::Matrix4f& vp) { draw(vp); };
}

void StarField::draw(Eigen::Matrix4f const& vp) const {
    if (!prog_) return;
    float alpha = std::clamp(-sun_elev_ * 3.0f, 0.0f, 1.0f);
    if (star_count_ <= 0 || alpha <= 0.01f) return;
    // Background layer: xyww in the shader pins depth to exactly 1.0, which
    // loses a GL_LESS test against the cleared buffer (Adreno's rounding
    // sometimes let it pass; Mesa never does). Skip depth entirely.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    prog_->use();
    GlProgram::uniform(vp_loc_, vp);
    glUniform1f(alpha_loc_,  alpha);
    glUniform1f(radius_loc_, radius_);
    glBindVertexArray(vao_);
    glDrawArrays(GL_POINTS, 0, star_count_);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

StarField::~StarField() {
    if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
}

StarField::StarField(StarField&& src) noexcept
    : prog_(std::move(src.prog_)),
      vao_(src.vao_),
      vp_loc_(src.vp_loc_),
      alpha_loc_(src.alpha_loc_),
      radius_loc_(src.radius_loc_),
      star_count_(src.star_count_),
      radius_(src.radius_),
      sun_elev_(src.sun_elev_) {
    src.vao_ = 0U;
}

StarField& StarField::operator=(StarField&& src) noexcept {
    if (this != &src) {
        if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
        prog_       = std::move(src.prog_);
        vao_        = src.vao_;  src.vao_ = 0U;
        vp_loc_     = src.vp_loc_;
        alpha_loc_  = src.alpha_loc_;
        radius_loc_ = src.radius_loc_;
        star_count_ = src.star_count_;
        radius_     = src.radius_;
        sun_elev_   = src.sun_elev_;
    }
    return *this;
}
