// Copyright 2025 Travis West
#include "rd_gpu.hpp"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <cstdio>
#include <vector>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

static const char* kVertSrc = R"(#version 300 es
in vec2 pos;
void main() { gl_Position = vec4(pos, 0.0, 1.0); }
)";

static const char* kFragSrc = R"(#version 300 es
precision highp float;
uniform sampler2D uState;
uniform float uFeed, uKill;
out vec4 fragColor;
void main() {
    ivec2 sz = textureSize(uState, 0);
    vec2 uv = gl_FragCoord.xy / vec2(sz);
    float dx = 1.0 / float(sz.x), dy = 1.0 / float(sz.y);
    vec2 c = texture(uState, uv).rg;
    vec2 l = texture(uState, uv + vec2(-dx, 0)).rg;
    vec2 r = texture(uState, uv + vec2( dx, 0)).rg;
    vec2 u = texture(uState, uv + vec2(0,  dy)).rg;
    vec2 d = texture(uState, uv + vec2(0, -dy)).rg;
    float U = c.r, V = c.g;
    float uvv = U * V * V;
    U += 0.2*(l.r+r.r+u.r+d.r - 4.0*U) - uvv + uFeed*(1.0-U);
    V += 0.1*(l.g+r.g+u.g+d.g - 4.0*V) + uvv - (uFeed+uKill)*V;
    fragColor = vec4(clamp(U,0.,1.), clamp(V,0.,1.), 0., 1.);
}
)";

static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char buf[512]; glGetShaderInfoLog(s, sizeof(buf), nullptr, buf); LOGE("rd_gpu shader: %s", buf); }
    return s;
}

void RdGpu::compile_shader() {
    GLuint vs = compile(GL_VERTEX_SHADER, kVertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFragSrc);
    prog_ = glCreateProgram();
    glAttachShader(prog_, vs); glAttachShader(prog_, fs);
    glBindAttribLocation(prog_, 0, "pos");
    glLinkProgram(prog_);
    glDeleteShader(vs); glDeleteShader(fs);
    GLint ok = 0; glGetProgramiv(prog_, GL_LINK_STATUS, &ok);
    if (!ok) { char buf[512]; glGetProgramInfoLog(prog_, sizeof(buf), nullptr, buf); LOGE("rd_gpu link: %s", buf); }
}

void RdGpu::seed_textures() {
    std::vector<float> data(static_cast<std::size_t>(width_ * height_) * 2, 0.f);
    for (int i = 0; i < width_ * height_; ++i) { data[2*i] = 1.f; }
    int cx = width_ / 2, cy = height_ / 2;
    for (int y = cy-2; y <= cy+2; ++y)
        for (int x = cx-2; x <= cx+2; ++x)
            if (x>=0 && x<width_ && y>=0 && y<height_)
                { data[2*(y*width_+x)] = 0.f; data[2*(y*width_+x)+1] = 1.f; }
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, tex_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width_, height_, 0, GL_RG, GL_FLOAT, data.data());
    }
}

void RdGpu::init(int w, int h) {
    width_ = w; height_ = h;
    compile_shader();
    glGenTextures(2, tex_);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, tex_[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    seed_textures();
    glGenFramebuffers(2, fbo_);
    for (int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_[i], 0);
    }
    static const float kQuad[] = {-1,-1, 1,-1, -1,1, 1,1};
    glGenBuffers(1, &quad_vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuad), kQuad, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ready_ = true;
}

void RdGpu::operator()(double /*time_s*/) {
    if (!ready_) return;
    GLint prev_fbo = 0; glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    glUseProgram(prog_);
    glUniform1i(glGetUniformLocation(prog_, "uState"), 0);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo_);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    int steps = static_cast<int>(inputs.steps_per_frame.value);
    for (int s = 0; s < steps; ++s) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_[ping_]);
        glViewport(0, 0, width_, height_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_[1-ping_]);
        glUniform1f(glGetUniformLocation(prog_, "uFeed"), inputs.feed.value);
        glUniform1f(glGetUniformLocation(prog_, "uKill"), inputs.kill.value);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        ping_ = 1 - ping_;
    }
    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
    outputs.concentration.value = {tex_[ping_], width_, height_, GL_RG32F, GL_LINEAR};
}
