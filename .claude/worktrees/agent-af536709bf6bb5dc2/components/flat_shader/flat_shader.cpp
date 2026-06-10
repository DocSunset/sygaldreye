#include "flat_shader.hpp"
#include "gl_program.hpp"
#include "log.hpp"

#define TAG "flat_shader"
#define LOGE(...) LOG_E(TAG, __VA_ARGS__)

namespace {
constexpr const char* const VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec4 aColor;
uniform mat4 uMVP;
flat out vec4 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

constexpr const char* const FRAG = R"(#version 300 es
precision mediump float;
flat in vec4 vColor;
out vec4 fragColor;
void main() { fragColor = vColor; }
)";
}

FlatShader::FlatShader() = default;

FlatShader::FlatShader(FlatShader&& src) noexcept
    : prog_(std::move(src.prog_)),
      mvp_loc_(src.mvp_loc_) {
    src.mvp_loc_ = -1;
}

FlatShader& FlatShader::operator=(FlatShader&& src) noexcept {
    if (this != &src) {
        prog_    = std::move(src.prog_);
        mvp_loc_ = src.mvp_loc_;
        src.mvp_loc_ = -1;
    }
    return *this;
}

void FlatShader::create() {
    auto result = GlProgram::build(VERT, FRAG);
    if (!result) { LOGE("shader build failed"); return; }
    prog_    = std::make_unique<GlProgram>(std::move(*result));
    mvp_loc_ = prog_->uniform_location("uMVP");
}

void FlatShader::use() const {
    prog_->use();
}

void FlatShader::set_mvp(const Eigen::Matrix4f& mvp) const {
    GlProgram::uniform(mvp_loc_, mvp);
}
