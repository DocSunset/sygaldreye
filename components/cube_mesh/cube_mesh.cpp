#include "cube_mesh.hpp"
#include <GLES3/gl3.h>
#include <android/log.h>

#define TAG "cube_mesh"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static const char* VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
uniform mat4 uMVP;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char* FRAG = R"(#version 300 es
precision mediump float;
in vec3 vColor;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, 1.0); }
)";

// 24 vertices: 4 per face, 6 faces. Each: x,y,z, r,g,b
// Faces ordered: +X, -X, +Y, -Y, +Z, -Z
static const float VERTS[24 * 6] = {
    // +X (right): red
     1,-1,-1, 1,0,0,
     1, 1,-1, 1,0,0,
     1, 1, 1, 1,0,0,
     1,-1, 1, 1,0,0,
    // -X (left): cyan
    -1,-1, 1, 0,1,1,
    -1, 1, 1, 0,1,1,
    -1, 1,-1, 0,1,1,
    -1,-1,-1, 0,1,1,
    // +Y (top): green
    -1, 1,-1, 0,1,0,
    -1, 1, 1, 0,1,0,
     1, 1, 1, 0,1,0,
     1, 1,-1, 0,1,0,
    // -Y (bottom): magenta
    -1,-1, 1, 1,0,1,
    -1,-1,-1, 1,0,1,
     1,-1,-1, 1,0,1,
     1,-1, 1, 1,0,1,
    // +Z (front): blue
    -1,-1, 1, 0,0,1,
     1,-1, 1, 0,0,1,
     1, 1, 1, 0,0,1,
    -1, 1, 1, 0,0,1,
    // -Z (back): yellow
     1,-1,-1, 1,1,0,
    -1,-1,-1, 1,1,0,
    -1, 1,-1, 1,1,0,
     1, 1,-1, 1,1,0,
};

// 6 faces × 2 triangles × 3 indices = 36
static const unsigned short IDX[36] = {
     0, 1, 2,  0, 2, 3,
     4, 5, 6,  4, 6, 7,
     8, 9,10,  8,10,11,
    12,13,14, 12,14,15,
    16,17,18, 16,18,19,
    20,21,22, 20,22,23,
};

void CubeMesh::init() {
    auto prog = GlProgram::build(VERT, FRAG);
    if (!prog) {
        LOGE("shader build failed");
        return;
    }
    prog_ = std::move(*prog);

    GLuint ebo = 0;
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTS), VERTS, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IDX), IDX, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);

    glBindVertexArray(0);
}

void CubeMesh::draw(const Eigen::Matrix4f& mvp) {
    prog_.use();
    prog_.uniform("uMVP", mvp);
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}
