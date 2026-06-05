#include "rgba_shader.hpp"
#include "gl_program.hpp"
#include <android/log.h>

#define TAG "rgba_shader"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace {
constexpr const char* const VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

constexpr const char* const FRAG = R"(#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() { fragColor = uColor; }
)";
}

RgbaShader::RgbaShader() = default;

RgbaShader::RgbaShader(RgbaShader&& src) noexcept
    : prog_(std::move(src.prog_)),
      mvp_loc_(src.mvp_loc_),
      color_loc_(src.color_loc_) {
    src.mvp_loc_   = -1;
    src.color_loc_ = -1;
}

RgbaShader& RgbaShader::operator=(RgbaShader&& src) noexcept {
    if (this != &src) {
        prog_      = std::move(src.prog_);
        mvp_loc_   = src.mvp_loc_;
        color_loc_ = src.color_loc_;
        src.mvp_loc_   = -1;
        src.color_loc_ = -1;
    }
    return *this;
}

void RgbaShader::create() {
    auto result = GlProgram::build(VERT, FRAG);
    if (!result) { LOGE("shader build failed"); return; }
    prog_      = std::make_unique<GlProgram>(std::move(*result));
    mvp_loc_   = prog_->uniform_location("uMVP");
    color_loc_ = prog_->uniform_location("uColor");
}

void RgbaShader::use() const {
    prog_->use();
}

void RgbaShader::set_mvp(const Eigen::Matrix4f& mvp) const {
    GlProgram::uniform(mvp_loc_, mvp);
}

void RgbaShader::set_color(const Eigen::Vector4f& color) const {
    glUniform4fv(color_loc_, 1, color.data());
}
