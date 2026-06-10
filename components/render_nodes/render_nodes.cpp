// Copyright 2026 Travis West
#include "render_nodes.hpp"
#include "gl_program.hpp"
#include <cstdio>

// ── render_target ───────────────────────────────────────────────────────────

void RenderTargetNode::operator()(double) {
    int w = int(inputs.width.value), h = int(inputs.height.value);
    if (fbo_ == 0 || w != w_ || h != h_) {
        if (fbo_) { glDeleteFramebuffers(1, &fbo_); glDeleteTextures(1, &color_);
                    glDeleteRenderbuffers(1, &depth_); }
        GLint prev_fbo = 0; glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
        glGenFramebuffers(1, &fbo_);
        glGenTextures(1, &color_);
        glGenRenderbuffers(1, &depth_);
        glBindTexture(GL_TEXTURE_2D, color_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::fprintf(stderr, "render_target: fbo incomplete\n");
        glBindFramebuffer(GL_FRAMEBUFFER, GLuint(prev_fbo));
        w_ = w; h_ = h;
    }

    GLint prev_fbo = 0; glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    GLint prev_vp[4]; glGetIntegerv(GL_VIEWPORT, prev_vp);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, w_, h_);
    glClearColor(inputs.bg_r.value, inputs.bg_g.value, inputs.bg_b.value, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    if (inputs.draw.value) inputs.draw.value(inputs.pv.value);
    glBindFramebuffer(GL_FRAMEBUFFER, GLuint(prev_fbo));
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);

    outputs.texture.value = {color_, w_, h_, GL_RGBA8, GL_LINEAR};
}

RenderTargetNode::~RenderTargetNode() {
    if (fbo_) { glDeleteFramebuffers(1, &fbo_); glDeleteTextures(1, &color_);
                glDeleteRenderbuffers(1, &depth_); }
}

// ── texture_view ────────────────────────────────────────────────────────────

namespace {
constexpr const char* kViewVert = R"(#version 300 es
precision mediump float;
out vec2 vUV;
void main() {
    // fullscreen triangle from gl_VertexID
    vec2 p = vec2((gl_VertexID == 1) ? 3.0 : -1.0,
                  (gl_VertexID == 2) ? 3.0 : -1.0);
    vUV = p * 0.5 + 0.5;
    gl_Position = vec4(p, 0.0, 1.0);
}
)";
constexpr const char* kViewFrag = R"(#version 300 es
precision mediump float;
in vec2 vUV;
uniform sampler2D uTex;
uniform float uAlpha;
out vec4 fragColor;
void main() {
    vec4 c = texture(uTex, vUV);
    fragColor = vec4(c.rgb, c.a * uAlpha);
}
)";
} // namespace

GlProgram* TextureViewNode::prog() {
    if (!prog_) {
        auto p = GlProgram::build(kViewVert, kViewFrag);
        if (!p) { std::fprintf(stderr, "texture_view: shader build failed\n"); return nullptr; }
        prog_ = std::make_unique<GlProgram>(std::move(*p));
    }
    return prog_.get();
}

void TextureViewNode::operator()(double) {
    tex_ = inputs.texture.value;
    outputs.render.value = [this](const Eigen::Matrix4f&) {
        if (!tex_.valid()) return;
        GlProgram* pr = prog();
        if (!pr) return;
        GLint prev_vao = 0; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prev_vao);
        if (vao_ == 0) glGenVertexArrays(1, &vao_);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        pr->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_.id);
        glUniform1i(pr->uniform_location("uTex"), 0);
        glUniform1f(pr->uniform_location("uAlpha"), inputs.alpha.value);
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(GLuint(prev_vao));
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    };
}
