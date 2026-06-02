#include "cube_mesh.hpp"
#include "gl_program.hpp"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <cassert>

#define TAG "cube_mesh"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define GL_CHECK(call) do { \
    (call); \
    GLenum _gl_err = glGetError(); \
    if (_gl_err != GL_NO_ERROR) { \
        LOGE("GL error 0x%x in " #call " (" __FILE__ ":%d)", (unsigned)_gl_err, __LINE__); \
    } \
} while(0)

namespace {
constexpr const char* const VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
uniform mat4 uMVP;
out vec3 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

constexpr const char* const FRAG = R"(#version 300 es
precision mediump float;
in vec3 vColor;
out vec4 fragColor;
void main() { fragColor = vec4(vColor, 1.0); }
)";

constexpr GLsizei VERTEX_STRIDE = 6 * static_cast<GLsizei>(sizeof(float));
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
const auto COLOR_OFFSET = reinterpret_cast<const void*>(3 * sizeof(float));
}

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

CubeMesh::CubeMesh() = default;

CubeMesh::~CubeMesh() {
    if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
    if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
}

CubeMesh::CubeMesh(CubeMesh&& src) noexcept
    : vao_(src.vao_), vbo_(src.vbo_), ebo_(src.ebo_),
      prog_(std::move(src.prog_)), mvp_loc_(src.mvp_loc_) {
    src.vao_ = 0U;
    src.vbo_ = 0U;
    src.ebo_ = 0U;
    src.mvp_loc_ = -1;
}

CubeMesh& CubeMesh::operator=(CubeMesh&& src) noexcept {
    if (this != &src) {
        if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
        if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
        if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
        vao_ = src.vao_; src.vao_ = 0U;
        vbo_ = src.vbo_; src.vbo_ = 0U;
        ebo_ = src.ebo_; src.ebo_ = 0U;
        prog_ = std::move(src.prog_);
        mvp_loc_ = src.mvp_loc_; src.mvp_loc_ = -1;
    }
    return *this;
}

void CubeMesh::init() {
    auto result = GlProgram::build(VERT, FRAG);
    if (!result) {
        LOGE("shader build failed");
        return;
    }
    prog_ = std::make_unique<GlProgram>(std::move(*result));
    mvp_loc_ = prog_->uniform_location("uMVP");

    GL_CHECK(glGenVertexArrays(1, &vao_));
    GL_CHECK(glGenBuffers(1, &vbo_));
    GL_CHECK(glGenBuffers(1, &ebo_));

    GL_CHECK(glBindVertexArray(vao_));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(VERTS), VERTS, GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(IDX), IDX, GL_STATIC_DRAW));

    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, nullptr));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, COLOR_OFFSET));

    glBindVertexArray(0);
}

void CubeMesh::begin_batch() const {
    assert(vao_ != 0U);
    prog_->use();
    glBindVertexArray(vao_);
}

void CubeMesh::end_batch() {
    glBindVertexArray(0);
}

void CubeMesh::draw(const Eigen::Matrix4f& mvp) const {
    assert(vao_ != 0U);
    prog_->uniform(mvp_loc_, mvp);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);
}
