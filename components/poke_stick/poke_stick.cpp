// Copyright 2026 Travis West
#include "poke_stick.hpp"
#include <GLES3/gl3.h>

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

// Unit cube, 12 triangles, centered at origin.
constexpr float kBox[] = {
    -1,-1,-1, 1,-1,-1, 1,1,-1,  -1,-1,-1, 1,1,-1, -1,1,-1,   // -z
    -1,-1, 1, 1,1, 1, 1,-1, 1,  -1,-1, 1, -1,1, 1, 1,1, 1,   // +z
    -1,-1,-1, -1,1,-1, -1,1,1,  -1,-1,-1, -1,1,1, -1,-1,1,   // -x
     1,-1,-1, 1,1,1, 1,1,-1,     1,-1,-1, 1,-1,1, 1,1,1,     // +x
    -1,-1,-1, 1,-1,1, 1,-1,-1,  -1,-1,-1, -1,-1,1, 1,-1,1,   // -y
    -1, 1,-1, 1,1,-1, 1,1,1,    -1, 1,-1, 1,1,1, -1,1,1,     // +y
};

GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}
} // namespace

void PokeStickNode::ensure_gl() {
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

void PokeStickNode::operator()(double) {
    Eigen::Vector3f pos = endpoints.pos.get();
    Eigen::Quaternionf rot = endpoints.rot.get().normalized();
    float len = endpoints.length.get(), rad = endpoints.radius.get();

    endpoints.tip_pos.value = pos + rot * Eigen::Vector3f(0.f, 0.f, -len);

    // Box centered halfway along the stick, pointing down controller -Z.
    Eigen::Affine3f model = Eigen::Translation3f(pos) * rot *
                            Eigen::Translation3f(0.f, 0.f, -len * 0.5f) *
                            Eigen::Scaling(rad, rad, len * 0.5f);
    Eigen::Matrix4f m = model.matrix();
    bool active = endpoints.active.get() > 0.5f;

    endpoints.render.value = [this, m, active](const Eigen::Matrix4f& vp) {
        ensure_gl();
        Eigen::Matrix4f mvp = vp * m;
        glUseProgram(program_);
        glUniformMatrix4fv(glGetUniformLocation(program_, "uMvp"),
                           1, GL_FALSE, mvp.data());
        const float gold[4] = {1.f, 0.8f, 0.2f, 1.f}, grey[4] = {0.8f, 0.8f, 0.85f, 1.f};
        glUniform4fv(glGetUniformLocation(program_, "uColor"), 1, active ? gold : grey);
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    };
}

#ifdef SYGALDREYE_PLUGIN
#include "eyeballs_node_abi.hpp"
EYEBALLS_EXPORT_NODE(PokeStickNode)
#endif
