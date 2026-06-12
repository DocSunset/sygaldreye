// Copyright 2026 Travis West
#include "poke_button.hpp"
#include <GLES3/gl3.h>
#include <cmath>

namespace {
const char* kVs = R"(#version 300 es
layout(location=0) in vec3 aPos;
uniform mat4 uMvp;
void main() { gl_Position = uMvp * vec4(aPos, 1.0); })";

const char* kFs = R"(#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 frag;
void main() { frag = uColor; })";

constexpr float kBox[] = {
    -1,-1,-1, 1,-1,-1, 1,1,-1,  -1,-1,-1, 1,1,-1, -1,1,-1,
    -1,-1, 1, 1,1, 1, 1,-1, 1,  -1,-1, 1, -1,1, 1, 1,1, 1,
    -1,-1,-1, -1,1,-1, -1,1,1,  -1,-1,-1, -1,1,1, -1,-1,1,
     1,-1,-1, 1,1,1, 1,1,-1,     1,-1,-1, 1,-1,1, 1,1,1,
    -1,-1,-1, 1,-1,1, 1,-1,-1,  -1,-1,-1, -1,-1,1, 1,-1,1,
    -1, 1,-1, 1,1,-1, 1,1,1,    -1, 1,-1, 1,1,1, -1,1,1,
};

GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}
} // namespace

void PokeButtonNode::ensure_gl() {
    if (gl_ready_) return;
    GLuint vs = compile(GL_VERTEX_SHADER, kVs), fs = compile(GL_FRAGMENT_SHADER, kFs);
    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glGenBuffers(1, &vbo_);
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kBox), kBox, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
    gl_ready_ = true;
}

void PokeButtonNode::operator()(double) {
    Eigen::Vector3f pos{endpoints.x.get(), endpoints.y.get(), endpoints.z.get()};
    float half = endpoints.size.get() * 0.5f;
    Eigen::Vector3f d = endpoints.tip.get() - pos;
    bool inside = std::abs(d.x()) < half && std::abs(d.y()) < half &&
                  std::abs(d.z()) < half;

    bool press = endpoints.press.get() > 0.5f;
    endpoints.pressed.triggered = inside && press && !prev_press_;
    prev_press_ = press;
    endpoints.hover.value = inside ? 1.f : 0.f;

    bool lit = inside && press;
    endpoints.render.value = [this, pos, half, inside, lit](const Eigen::Matrix4f& vp) {
        ensure_gl();
        Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
        m(0, 0) = m(1, 1) = m(2, 2) = half;
        m(0, 3) = pos.x(); m(1, 3) = pos.y(); m(2, 3) = pos.z();
        Eigen::Matrix4f mvp = vp * m;
        glUseProgram(program_);
        glUniformMatrix4fv(glGetUniformLocation(program_, "uMvp"), 1, GL_FALSE, mvp.data());
        const float idle[4]  = {0.25f, 0.3f, 0.45f, 1.f};
        const float hov[4]   = {0.4f, 0.6f, 1.f, 1.f};
        const float fire[4]  = {1.f, 0.8f, 0.2f, 1.f};
        glUniform4fv(glGetUniformLocation(program_, "uColor"), 1,
                     lit ? fire : (inside ? hov : idle));
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    };
}

#ifdef SYGALDREYE_PLUGIN
#include "eyeballs_node_abi.hpp"
EYEBALLS_EXPORT_NODE(PokeButtonNode)
#endif
