#include "cube_geometry.hpp"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <cassert>

#define TAG "cube_geometry"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define GL_CHECK(call) do { \
    (call); \
    GLenum _gl_err = glGetError(); \
    if (_gl_err != GL_NO_ERROR) { \
        LOGE("GL error 0x%x in " #call " (" __FILE__ ":%d)", (unsigned)_gl_err, __LINE__); \
    } \
} while(0)

namespace {
constexpr GLsizei VERTEX_STRIDE = 6 * static_cast<GLsizei>(sizeof(float));
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
const auto COLOR_OFFSET = reinterpret_cast<const void*>(3 * sizeof(float));
}

// 24 vertices: 4 per face, 6 faces. Each: x,y,z, r,g,b
static const float VERTS[24 * 6] = {
    // +X red, -X cyan, +Y green, -Y magenta, +Z blue, -Z yellow
     1,-1,-1, 1,0,0,  1, 1,-1, 1,0,0,  1, 1, 1, 1,0,0,  1,-1, 1, 1,0,0,
    -1,-1, 1, 0,1,1, -1, 1, 1, 0,1,1, -1, 1,-1, 0,1,1, -1,-1,-1, 0,1,1,
    -1, 1,-1, 0,1,0, -1, 1, 1, 0,1,0,  1, 1, 1, 0,1,0,  1, 1,-1, 0,1,0,
    -1,-1, 1, 1,0,1, -1,-1,-1, 1,0,1,  1,-1,-1, 1,0,1,  1,-1, 1, 1,0,1,
    -1,-1, 1, 0,0,1,  1,-1, 1, 0,0,1,  1, 1, 1, 0,0,1, -1, 1, 1, 0,0,1,
     1,-1,-1, 1,1,0, -1,-1,-1, 1,1,0, -1, 1,-1, 1,1,0,  1, 1,-1, 1,1,0,
};

static const unsigned short IDX[36] = {
     0, 1, 2,  0, 2, 3,
     4, 5, 6,  4, 6, 7,
     8, 9,10,  8,10,11,
    12,13,14, 12,14,15,
    16,17,18, 16,18,19,
    20,21,22, 20,22,23,
};

CubeGeometry::CubeGeometry() = default;

CubeGeometry::~CubeGeometry() {
    if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
    if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
}

CubeGeometry::CubeGeometry(CubeGeometry&& src) noexcept
    : vao_(src.vao_), vbo_(src.vbo_), ebo_(src.ebo_) {
    src.vao_ = 0U; src.vbo_ = 0U; src.ebo_ = 0U;
}

CubeGeometry& CubeGeometry::operator=(CubeGeometry&& src) noexcept {
    if (this != &src) {
        if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
        if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
        if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
        vao_ = src.vao_; src.vao_ = 0U;
        vbo_ = src.vbo_; src.vbo_ = 0U;
        ebo_ = src.ebo_; src.ebo_ = 0U;
    }
    return *this;
}

void CubeGeometry::init() {
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

void CubeGeometry::bind() const {
    assert(vao_ != 0U);
    glBindVertexArray(vao_);
}

void CubeGeometry::unbind() { glBindVertexArray(0); }

void CubeGeometry::draw_elements() {
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);
}
