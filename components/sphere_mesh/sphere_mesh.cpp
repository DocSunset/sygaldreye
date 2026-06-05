#include "sphere_mesh.hpp"
#include <android/log.h>

#define TAG "sphere_mesh"
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
const auto NORMAL_OFFSET = reinterpret_cast<const void*>(3 * sizeof(float));
}

SphereMesh SphereMesh::create(const SphereGeometry& geo) {
    SphereMesh mesh;
    mesh.index_count_ = static_cast<GLsizei>(geo.indices.size());

    GL_CHECK(glGenVertexArrays(1, &mesh.vao_));
    GL_CHECK(glGenBuffers(1, &mesh.vbo_));
    GL_CHECK(glGenBuffers(1, &mesh.ebo_));
    GL_CHECK(glBindVertexArray(mesh.vao_));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo_));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(geo.vertices.size() * sizeof(SphereVertex)),
        geo.vertices.data(), GL_STATIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo_));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(geo.indices.size() * sizeof(uint16_t)),
        geo.indices.data(), GL_STATIC_DRAW));

    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, nullptr));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_STRIDE, NORMAL_OFFSET));

    glBindVertexArray(0);
    return mesh;
}

SphereMesh::~SphereMesh() {
    if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
    if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
}

SphereMesh::SphereMesh(SphereMesh&& src) noexcept
    : vao_(src.vao_), vbo_(src.vbo_), ebo_(src.ebo_), index_count_(src.index_count_) {
    src.vao_ = 0U; src.vbo_ = 0U; src.ebo_ = 0U; src.index_count_ = 0;
}

SphereMesh& SphereMesh::operator=(SphereMesh&& src) noexcept {
    if (this != &src) {
        if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
        if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
        if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
        vao_ = src.vao_; src.vao_ = 0U;
        vbo_ = src.vbo_; src.vbo_ = 0U;
        ebo_ = src.ebo_; src.ebo_ = 0U;
        index_count_ = src.index_count_; src.index_count_ = 0;
    }
    return *this;
}

void SphereMesh::draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}
