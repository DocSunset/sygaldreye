#include "wind_shader.hpp"
#include "gl_program.hpp"
#include <android/log.h>

#define TAG "wind_shader"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace {
// Vertex layout matches TriMesh: loc 0=position(vec3), loc 1=normal(vec3), loc 2=color(vec4).
// aColor.a carries the sway mask (0=anchored, 1=free); RGB used for fragment color.
constexpr const char* const VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=2) in vec4 aColor;
uniform mat4  uMVP;
uniform float uTime;
uniform vec3  uWindDir;
uniform float uWindStrength;
flat out vec4 vColor;
void main() {
    float sway = aColor.a;
    float disp = uWindStrength * sway
               * sin(uTime * 2.1 + dot(aPos.xz, uWindDir.xz) * 3.5);
    vec3 pos = aPos + uWindDir * disp;
    gl_Position = uMVP * vec4(pos, 1.0);
    vColor = vec4(aColor.rgb, 1.0);
}
)";

constexpr const char* const FRAG = R"(#version 300 es
precision mediump float;
flat in vec4 vColor;
out vec4 fragColor;
void main() { fragColor = vColor; }
)";
}

WindShader::WindShader() = default;

WindShader::WindShader(WindShader&& src) noexcept
    : prog_(std::move(src.prog_)),
      mvp_loc_(src.mvp_loc_),
      time_loc_(src.time_loc_),
      wind_dir_loc_(src.wind_dir_loc_),
      wind_str_loc_(src.wind_str_loc_) {
    src.mvp_loc_ = src.time_loc_ = src.wind_dir_loc_ = src.wind_str_loc_ = -1;
}

WindShader& WindShader::operator=(WindShader&& src) noexcept {
    if (this != &src) {
        prog_         = std::move(src.prog_);
        mvp_loc_      = src.mvp_loc_;
        time_loc_     = src.time_loc_;
        wind_dir_loc_ = src.wind_dir_loc_;
        wind_str_loc_ = src.wind_str_loc_;
        src.mvp_loc_ = src.time_loc_ = src.wind_dir_loc_ = src.wind_str_loc_ = -1;
    }
    return *this;
}

void WindShader::create() {
    auto result = GlProgram::build(VERT, FRAG);
    if (!result) { LOGE("shader build failed"); return; }
    prog_         = std::make_unique<GlProgram>(std::move(*result));
    mvp_loc_      = prog_->uniform_location("uMVP");
    time_loc_     = prog_->uniform_location("uTime");
    wind_dir_loc_ = prog_->uniform_location("uWindDir");
    wind_str_loc_ = prog_->uniform_location("uWindStrength");
}

void WindShader::use() const { prog_->use(); }

void WindShader::set_mvp(const Eigen::Matrix4f& mvp) const {
    GlProgram::uniform(mvp_loc_, mvp);
}

void WindShader::set_time(float t) const {
    glUniform1f(time_loc_, t);
}

void WindShader::set_wind(Eigen::Vector3f dir, float strength) const {
    glUniform3f(wind_dir_loc_, dir.x(), dir.y(), dir.z());
    glUniform1f(wind_str_loc_, strength);
}
