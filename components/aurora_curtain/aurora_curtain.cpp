// Copyright 2026 Travis West
#include "aurora_curtain.hpp"
#include <cstdio>
#include <vector>

namespace {
// Same ripple math the monolithic aurora used; per-curtain uniforms are
// now node inputs.
constexpr const char* VERT = R"(#version 300 es
precision highp float;
layout(location=0) in vec2 aUV;
uniform mat4  uVP;
uniform float uTime, uXOffset, uPhase, uFreq, uSpeed;
uniform float uRippleAmp, uAltBase, uAltHeight, uDepthExtent;
uniform vec3  uColor;
out float vAlpha;
out vec3  vColor;
void main() {
    float u = aUV.x, v = aUV.y;
    float z = (u - 0.5) * uDepthExtent;
    float bottom_weight = 1.0 - v * 0.8;
    float w1 = sin(u * uFreq * 4.712 + uTime * uSpeed        + uPhase);
    float w2 = sin(u * uFreq * 7.854 - uTime * uSpeed * 0.61 + uPhase * 1.4);
    float w3 = sin(u * uFreq * 2.618 + uTime * uSpeed * 1.37 + uPhase * 0.5);
    float ripple = (w1 * 0.5 + w2 * 0.3 + w3 * 0.2) * uRippleAmp * bottom_weight;
    float sway = sin(uTime * uSpeed * 0.28 + uPhase * 0.8) * uRippleAmp * 0.25;
    gl_Position = uVP * vec4(uXOffset + ripple + sway, uAltBase + v * uAltHeight, z, 1.0);
    float bright = sin(u * uFreq * 3.5 + uTime * uSpeed * 0.9 + uPhase) * 0.25 + 0.75;
    float pulse  = sin(uTime * uSpeed * 0.4 + uPhase * 1.1) * 0.12 + 0.88;
    vAlpha = clamp(v * v * bright * pulse * 0.9, 0.0, 1.0);
    vColor = uColor;
}
)";
constexpr const char* FRAG = R"(#version 300 es
precision mediump float;
in float vAlpha;
in vec3  vColor;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, vAlpha); }
)";
constexpr int kSegW = 60, kSegH = 24;
} // namespace

void AuroraCurtainNode::init_gl() {
    auto prog = GlProgram::build(VERT, FRAG);
    if (!prog) { std::fprintf(stderr, "aurora_curtain: shader build failed\n"); return; }
    prog_ = std::make_unique<GlProgram>(std::move(*prog));

    std::vector<float> uvs;
    uvs.reserve(size_t(kSegW + 1) * (kSegH + 1) * 2);
    for (int j = 0; j <= kSegH; ++j)
        for (int i = 0; i <= kSegW; ++i) {
            uvs.push_back(float(i) / kSegW);
            uvs.push_back(float(j) / kSegH);
        }
    std::vector<uint32_t> idx;
    idx.reserve(size_t(kSegW) * kSegH * 6);
    for (int j = 0; j < kSegH; ++j)
        for (int i = 0; i < kSegW; ++i) {
            uint32_t base = uint32_t(j * (kSegW + 1) + i), w = kSegW + 1;
            idx.insert(idx.end(), {base, base + w, base + 1,
                                   base + 1, base + w, base + w + 1});
        }
    index_count_ = GLsizei(idx.size());
    geom_ = GlGeometry::create(uvs, idx, {{0, 2, 2 * sizeof(float), 0}});
}

void AuroraCurtainNode::operator()(double time_s) {
    if (!prog_) init_gl();
    time_s_ = float(time_s);
    outputs.render.value = [this](const Eigen::Matrix4f& vp) {
        if (!prog_) return;
        prog_->use();
        GlProgram::uniform(prog_->uniform_location("uVP"), vp);
        glUniform1f(prog_->uniform_location("uTime"),        time_s_);
        glUniform1f(prog_->uniform_location("uXOffset"),     inputs.x_offset.value);
        glUniform1f(prog_->uniform_location("uPhase"),       inputs.phase.value);
        glUniform1f(prog_->uniform_location("uFreq"),        inputs.freq.value);
        glUniform1f(prog_->uniform_location("uSpeed"),       inputs.speed.value);
        glUniform1f(prog_->uniform_location("uRippleAmp"),   inputs.ripple_amp.value);
        glUniform1f(prog_->uniform_location("uAltBase"),     inputs.alt_base.value);
        glUniform1f(prog_->uniform_location("uAltHeight"),   inputs.alt_height.value);
        glUniform1f(prog_->uniform_location("uDepthExtent"), inputs.depth.value);
        glUniform3f(prog_->uniform_location("uColor"),
                    inputs.r.value, inputs.g.value, inputs.b.value);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDisable(GL_DEPTH_TEST);
        geom_.draw_elements(GL_TRIANGLES, index_count_);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    };
}
