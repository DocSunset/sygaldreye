// Copyright 2025 Travis West
#include "aurora.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Core>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <utility>

namespace {

// Curtains run along Z (depth) and hang down from altitude.
// u in [0,1] along Z (depth), v in [0,1] bottom=0 top=1.
// X position = x_offset + ripple(u, time).
// Y = altitude_base + v * altitude_height (hangs down from top).
constexpr const char* VERT = R"(#version 300 es
precision highp float;
layout(location=0) in vec2 aUV;

uniform mat4  uVP;
uniform float uTime;
uniform float uXOffset;
uniform float uPhase;
uniform float uFreq;
uniform float uSpeed;
uniform float uRippleAmp;
uniform float uAltBase;
uniform float uAltHeight;
uniform float uDepthExtent;

out float vAlpha;
uniform vec3 uColor;
out vec3  vColor;

void main() {
    float u = aUV.x;  // 0..1 along depth (Z)
    float v = aUV.y;  // 0..1 bottom..top

    // Z runs from -depthExtent/2 to +depthExtent/2
    float z = (u - 0.5) * uDepthExtent;

    // X: base offset + wave displacement, more at bottom
    float bottom_weight = 1.0 - v * 0.8;
    float w1 = sin(u * uFreq * 4.712 + uTime * uSpeed         + uPhase);
    float w2 = sin(u * uFreq * 7.854 - uTime * uSpeed * 0.61  + uPhase * 1.4);
    float w3 = sin(u * uFreq * 2.618 + uTime * uSpeed * 1.37  + uPhase * 0.5);
    float ripple = (w1 * 0.5 + w2 * 0.3 + w3 * 0.2) * uRippleAmp * bottom_weight;

    // Gentle whole-curtain sway
    float sway = sin(uTime * uSpeed * 0.28 + uPhase * 0.8) * uRippleAmp * 0.25;

    float x = uXOffset + ripple + sway;
    float y = uAltBase + v * uAltHeight;

    gl_Position = uVP * vec4(x, y, z, 1.0);

    // Alpha: quadratic from bottom, modulated by horizontal ripple and pulse
    float base_alpha = v * v;
    float bright = sin(u * uFreq * 3.5 + uTime * uSpeed * 0.9 + uPhase) * 0.25 + 0.75;
    float pulse   = sin(uTime * uSpeed * 0.4 + uPhase * 1.1) * 0.12 + 0.88;
    vAlpha = clamp(base_alpha * bright * pulse * 0.9, 0.0, 1.0);

    vColor = uColor;
}
)";

constexpr const char* FRAG = R"(#version 300 es
precision mediump float;
in float vAlpha;
in vec3  vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, vAlpha);
}
)";

} // namespace

Aurora Aurora::create_default() { return create({}); }

Aurora Aurora::create(AuroraParams const& p) {
    Aurora a{RawTag{}};
    a.params_ = p;

    auto prog = GlProgram::build(VERT, FRAG);
    if (!prog) { std::fprintf(stderr, "aurora: shader build failed\n"); return a; }
    a.prog_ = std::make_unique<GlProgram>(std::move(*prog));

    a.vp_loc_          = a.prog_->uniform_location("uVP");
    a.time_loc_        = a.prog_->uniform_location("uTime");
    a.color_loc_       = a.prog_->uniform_location("uColor");
    a.x_offset_loc_    = a.prog_->uniform_location("uXOffset");
    a.phase_loc_       = a.prog_->uniform_location("uPhase");
    a.freq_loc_        = a.prog_->uniform_location("uFreq");
    a.speed_loc_       = a.prog_->uniform_location("uSpeed");
    a.ramp_amp_loc_    = a.prog_->uniform_location("uRippleAmp");
    a.alt_base_loc_    = a.prog_->uniform_location("uAltBase");
    a.alt_height_loc_  = a.prog_->uniform_location("uAltHeight");
    a.depth_loc_       = a.prog_->uniform_location("uDepthExtent");

    // Build shared UV grid
    int sw = p.curtain_segments_w, sh = p.curtain_segments_h;
    std::vector<float> uvs;
    uvs.reserve(static_cast<size_t>((sw + 1) * (sh + 1) * 2));
    for (int j = 0; j <= sh; ++j)
        for (int i = 0; i <= sw; ++i) {
            uvs.push_back(float(i) / float(sw));
            uvs.push_back(float(j) / float(sh));
        }

    // Shared index buffer
    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>(sw * sh * 6));
    for (int j = 0; j < sh; ++j)
        for (int i = 0; i < sw; ++i) {
            uint32_t base = static_cast<uint32_t>(j * (sw + 1) + i);
            uint32_t w    = static_cast<uint32_t>(sw + 1);
            indices.push_back(base);
            indices.push_back(base + w);
            indices.push_back(base + 1);
            indices.push_back(base + 1);
            indices.push_back(base + w);
            indices.push_back(base + w + 1);
        }
    a.index_count_ = static_cast<GLsizei>(indices.size());

    glGenBuffers(1, &a.ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a.ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                 indices.data(), GL_STATIC_DRAW);

    // Per-curtain colors (wrap via modulo for any curtain count)
    const Eigen::Vector3f colors[] = {
        {0.1f,  0.9f,  0.3f},   // classic green
        {0.5f,  0.1f,  0.9f},   // violet
        {0.1f,  0.7f,  0.9f},   // cyan
        {0.05f, 0.8f,  0.5f},   // teal-green
        {0.4f,  0.05f, 0.8f},   // blue-violet
        {0.15f, 0.95f, 0.4f},   // bright green
        {0.3f,  0.05f, 0.7f},   // deep violet
    };
    constexpr int num_colors = int(sizeof(colors) / sizeof(colors[0]));

    auto lcg = [](uint32_t& s) -> float {
        s = s * 1664525u + 1013904223u;
        return float(s >> 8) / float(1u << 24);
    };
    uint32_t rng = 0xA5A5A5A5u;

    a.curtains_.resize(static_cast<size_t>(p.curtain_count));
    // Distribute curtains uniformly in X, with small random jitter
    float x_step = p.curtain_width * 0.6f / float(p.curtain_count > 1 ? p.curtain_count - 1 : 1);
    float x_start = -p.curtain_width * 0.3f;
    for (int ci = 0; ci < p.curtain_count; ++ci) {
        auto& c = a.curtains_[static_cast<size_t>(ci)];
        c.color    = colors[static_cast<size_t>(ci % num_colors)];
        float jitter = (lcg(rng) - 0.5f) * x_step * 0.4f;
        c.x_offset = x_start + ci * x_step + jitter;
        c.phase    = lcg(rng) * 6.283f;
        c.freq     = 0.7f + lcg(rng) * 0.9f;
        c.speed    = 0.6f + lcg(rng) * 0.8f;

        glGenVertexArrays(1, &c.vao);
        glBindVertexArray(c.vao);

        glGenBuffers(1, &c.vbo);
        glBindBuffer(GL_ARRAY_BUFFER, c.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(uvs.size() * sizeof(float)),
                     uvs.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, a.ebo_);
        glBindVertexArray(0);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return a;
}

void Aurora::update(float time_s) { time_s_ = time_s; }

void Aurora::sync_params() {
    params_.curtain_width     = inputs.curtain_width.value;
    params_.altitude_base     = inputs.altitude_base.value;
    params_.altitude_height   = inputs.altitude_height.value;
    params_.ripple_amplitude  = inputs.ripple_amplitude.value;
    params_.ripple_speed      = inputs.ripple_speed.value;
}

void Aurora::draw(Eigen::Matrix4f const& vp) const {
    if (!prog_) return;

    prog_->use();
    GlProgram::uniform(vp_loc_, vp);
    glUniform1f(time_loc_,       time_s_);
    glUniform1f(ramp_amp_loc_,   params_.ripple_amplitude);
    glUniform1f(alt_base_loc_,   params_.altitude_base);
    glUniform1f(alt_height_loc_, params_.altitude_height);
    glUniform1f(depth_loc_,      params_.curtain_width);  // reuse width as depth extent

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDisable(GL_DEPTH_TEST);

    for (auto const& c : curtains_) {
        glUniform3fv(color_loc_,   1, c.color.data());
        glUniform1f(x_offset_loc_, c.x_offset);
        glUniform1f(phase_loc_,    c.phase);
        glUniform1f(freq_loc_,     c.freq);
        glUniform1f(speed_loc_,    c.speed * params_.ripple_speed);

        glBindVertexArray(c.vao);
        glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

Aurora::Aurora() { *this = create_default(); }

Aurora::~Aurora() {
    for (auto& c : curtains_) {
        if (c.vao) { glDeleteVertexArrays(1, &c.vao); c.vao = 0; }
        if (c.vbo) { glDeleteBuffers(1, &c.vbo);      c.vbo = 0; }
    }
    if (ebo_) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }
}

Aurora::Aurora(Aurora&& other) noexcept
    : params_(other.params_)
    , prog_(std::move(other.prog_))
    , curtains_(std::move(other.curtains_))
    , ebo_(std::exchange(other.ebo_, 0))
    , index_count_(other.index_count_)
    , time_s_(other.time_s_)
    , vp_loc_(other.vp_loc_)
    , time_loc_(other.time_loc_)
    , color_loc_(other.color_loc_)
    , x_offset_loc_(other.x_offset_loc_)
    , phase_loc_(other.phase_loc_)
    , freq_loc_(other.freq_loc_)
    , speed_loc_(other.speed_loc_)
    , ramp_amp_loc_(other.ramp_amp_loc_)
    , alt_base_loc_(other.alt_base_loc_)
    , alt_height_loc_(other.alt_height_loc_)
    , depth_loc_(other.depth_loc_)
{}

Aurora& Aurora::operator=(Aurora&& other) noexcept {
    if (this != &other) {
        this->~Aurora();
        new (this) Aurora(std::move(other));
    }
    return *this;
}
