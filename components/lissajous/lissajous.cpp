// Copyright 2025 Travis West
#include "lissajous.hpp"
#include <GLES3/gl3.h>
#include <cmath>

namespace {

// HSV to RGB, all in [0,1]
static Eigen::Vector3f hsv2rgb(float h, float s, float v) {
    float h6 = h * 6.0f;
    int   i  = static_cast<int>(h6) % 6;
    float f  = h6 - std::floor(h6);
    float p  = v * (1.0f - s);
    float q  = v * (1.0f - f * s);
    float t  = v * (1.0f - (1.0f - f) * s);
    switch (i) {
        case 0: return {v, t, p};
        case 1: return {q, v, p};
        case 2: return {p, v, t};
        case 3: return {p, q, v};
        case 4: return {t, p, v};
        default: return {v, p, q};
    }
}

constexpr const char* VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
uniform mat4 uMVP;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

constexpr const char* FRAG = R"(#version 300 es
precision mediump float;
in vec3 vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 1.0);
}
)";

} // namespace

Lissajous Lissajous::create_default() { return create({}); }

Lissajous Lissajous::create(LissajousParams const& p) {
    Lissajous l{RawTag{}};
    l.params_ = p;
    l.vbo_data_.resize(static_cast<size_t>(p.samples) * 6);

    l.geom_ = GlGeometry::create(
        std::span<const float>{l.vbo_data_},
        {},
        {{0, 3, 6 * sizeof(float), 0}, {1, 3, 6 * sizeof(float), 3 * sizeof(float)}},
        GL_DYNAMIC_DRAW);

    auto prog = GlProgram::build(VERT, FRAG);
    if (prog) {
        l.prog_    = std::make_unique<GlProgram>(std::move(*prog));
        l.mvp_loc_ = l.prog_->uniform_location("uMVP");
    }
    return l;
}

void Lissajous::update(float time_s) {
    auto& p = params_;
    // Animate phase_x over time (full cycle every 16s)
    float phase = p.phase_x + time_s * (2.0f * static_cast<float>(M_PI) / 16.0f);
    // z freq drifts slowly
    float fz = p.freq_z + std::sin(time_s * 0.1f) * 0.3f;

    int n = p.samples;
    for (int i = 0; i < n; ++i) {
        float t   = static_cast<float>(i) / static_cast<float>(n - 1) * 2.0f * static_cast<float>(M_PI);
        float x   = p.amp * std::sin(p.freq_x * t + phase);
        float y   = p.amp * std::sin(p.freq_y * t);
        float z   = p.amp * 0.3f * std::sin(fz * t);
        float hue = static_cast<float>(i) / static_cast<float>(n);
        auto rgb  = hsv2rgb(hue, 0.85f, 1.0f);
        size_t base = static_cast<size_t>(i) * 6;
        vbo_data_[base+0] = x; vbo_data_[base+1] = y; vbo_data_[base+2] = z;
        vbo_data_[base+3] = rgb.x(); vbo_data_[base+4] = rgb.y(); vbo_data_[base+5] = rgb.z();
    }

    geom_.update_verts(vbo_data_);
}

void Lissajous::sync_params() {
    params_.freq_x  = inputs.freq_x.value;
    params_.freq_y  = inputs.freq_y.value;
    params_.freq_z  = inputs.freq_z.value;
    params_.phase_x = inputs.phase_x.value;
    params_.amp     = inputs.amp.value;
    params_.samples = static_cast<int>(inputs.samples.value);
}

void Lissajous::draw(Eigen::Matrix4f const& mvp) const {
    if (!prog_) return;
    prog_->use();
    GlProgram::uniform(mvp_loc_, mvp);
    geom_.draw_arrays(GL_LINE_STRIP, static_cast<GLsizei>(params_.samples));
}
