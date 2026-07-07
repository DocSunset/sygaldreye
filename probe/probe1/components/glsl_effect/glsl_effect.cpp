// Copyright 2026 Travis West
#include "glsl_effect.hpp"
#include <cstdio>

namespace {
constexpr const char* kVert = R"(#version 300 es
precision mediump float;
out vec2 vUV;
void main() {
    vec2 p = vec2((gl_VertexID == 1) ? 3.0 : -1.0,
                  (gl_VertexID == 2) ? 3.0 : -1.0);
    vUV = p * 0.5 + 0.5;
    gl_Position = vec4(p, 0.0, 1.0);
}
)";
constexpr const char* kFragHeader = R"(#version 300 es
precision mediump float;
in vec2 vUV;
uniform sampler2D uTex;
uniform sampler2D uTex2;
uniform float uTime, uA, uB, uC, uD;
out vec4 fragColor;
vec4 effect(vec2 uv) {
)";
constexpr const char* kFragFooter = R"(
}
void main() { fragColor = effect(vUV); }
)";
constexpr const char* kPassthrough = "return texture(uTex, uv);";
} // namespace

bool GlslEffectNode::ensure_program() {
    const std::string& code = endpoints.code.get();
    if (prog_ && code == compiled_code_) return true;
    if (compile_failed_ && code == compiled_code_) return false;

    std::string body = code.empty() ? kPassthrough : code;
    std::string src  = std::string(kFragHeader) + body + kFragFooter;
    auto p = GlProgram::build(kVert, src.c_str());
    compiled_code_ = code;
    if (!p) {
        // Keep the previous program (if any) running — a typo mid-livecode
        // shouldn't black the chain out.
        std::fprintf(stderr, "glsl_effect: compile failed; keeping last good\n");
        compile_failed_ = true;
        return prog_ != nullptr;
    }
    compile_failed_ = false;
    prog_ = std::make_unique<GlProgram>(std::move(*p));
    return true;
}

bool GlslEffectNode::ensure_target(int w, int h) {
    if (fbo_[0] && w == w_ && h == h_) return true;
    if (fbo_[0]) { glDeleteFramebuffers(2, fbo_); glDeleteTextures(2, color_); }
    GLint prev = 0; glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev);
    glGenFramebuffers(2, fbo_);
    glGenTextures(2, color_);
    bool ok = true;
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, color_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_[i], 0);
        ok &= glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    }
    if (!ok) std::fprintf(stderr, "glsl_effect: fbo incomplete\n");
    glBindFramebuffer(GL_FRAMEBUFFER, GLuint(prev));
    w_ = w; h_ = h;
    return ok;
}

void GlslEffectNode::operator()(double time_s) {
    if (!ensure_program()) return;
    int w = int(endpoints.width.get()), h = int(endpoints.height.get());
    if (!ensure_target(w, h)) return;

    GLint prev_fbo = 0;  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    GLint prev_vao = 0;  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prev_vao);
    GLint prev_vp[4];    glGetIntegerv(GL_VIEWPORT, prev_vp);

    if (vao_ == 0) glGenVertexArrays(1, &vao_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_[ping_]);
    glViewport(0, 0, w_, h_);
    glDisable(GL_DEPTH_TEST);
    prog_->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, endpoints.texture.get().id);  // 0 is fine: black
    glUniform1i(prog_->uniform_location("uTex"), 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, endpoints.texture2.get().id);
    glUniform1i(prog_->uniform_location("uTex2"), 1);
    glActiveTexture(GL_TEXTURE0);
    glUniform1f(prog_->uniform_location("uTime"), float(time_s));
    glUniform1f(prog_->uniform_location("uA"), endpoints.a.get());
    glUniform1f(prog_->uniform_location("uB"), endpoints.b.get());
    glUniform1f(prog_->uniform_location("uC"), endpoints.c.get());
    glUniform1f(prog_->uniform_location("uD"), endpoints.d.get());
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(GLuint(prev_vao));
    glBindFramebuffer(GL_FRAMEBUFFER, GLuint(prev_fbo));
    glViewport(prev_vp[0], prev_vp[1], prev_vp[2], prev_vp[3]);
    glEnable(GL_DEPTH_TEST);

    endpoints.texture_out.value = {color_[ping_], w_, h_, GL_RGBA8, GL_LINEAR};
    ping_ = 1 - ping_;
}

GlslEffectNode::~GlslEffectNode() {
    if (fbo_[0]) { glDeleteFramebuffers(2, fbo_); glDeleteTextures(2, color_); }
    if (vao_) glDeleteVertexArrays(1, &vao_);
}
