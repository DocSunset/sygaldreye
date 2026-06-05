#include "tri_mesh.hpp"
#include <android/log.h>

#define TAG "tri_mesh"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#define GL_CHECK(call) do { \
    (call); \
    GLenum _e = glGetError(); \
    if (_e != GL_NO_ERROR) LOGE("GL error 0x%x in " #call " (" __FILE__ ":%d)", (unsigned)_e, __LINE__); \
} while(0)

namespace {
constexpr GLsizei STRIDE = static_cast<GLsizei>(sizeof(TriVertex));
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
const auto NORMAL_OFFSET = reinterpret_cast<const void*>(offsetof(TriVertex, normal));
// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
const auto COLOR_OFFSET  = reinterpret_cast<const void*>(offsetof(TriVertex, color));
}

TriMesh TriMesh::create(TriMeshData const& data) {
    TriMesh m;
    m.index_count_  = static_cast<GLsizei>(data.indices.size());
    m.vertex_count_ = static_cast<GLsizei>(data.vertices.size());

    GL_CHECK(glGenVertexArrays(1, &m.vao_));
    GL_CHECK(glGenBuffers(1, &m.vbo_));
    GL_CHECK(glGenBuffers(1, &m.ebo_));
    GL_CHECK(glBindVertexArray(m.vao_));

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, m.vbo_));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(data.vertices.size() * sizeof(TriVertex)),
        data.vertices.data(), GL_DYNAMIC_DRAW));

    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo_));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(data.indices.size() * sizeof(uint32_t)),
        data.indices.data(), GL_DYNAMIC_DRAW));

    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, nullptr));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, STRIDE, NORMAL_OFFSET));
    GL_CHECK(glEnableVertexAttribArray(2));
    GL_CHECK(glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, STRIDE, COLOR_OFFSET));

    glBindVertexArray(0);
    return m;
}

TriMesh::~TriMesh() {
    if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
    if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
}

TriMesh::TriMesh(TriMesh&& src) noexcept
    : vao_(src.vao_), vbo_(src.vbo_), ebo_(src.ebo_),
      index_count_(src.index_count_), vertex_count_(src.vertex_count_) {
    src.vao_ = 0U; src.vbo_ = 0U; src.ebo_ = 0U;
    src.index_count_ = 0; src.vertex_count_ = 0;
}

TriMesh& TriMesh::operator=(TriMesh&& src) noexcept {
    if (this != &src) {
        if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
        if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
        if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
        vao_ = src.vao_; src.vao_ = 0U;
        vbo_ = src.vbo_; src.vbo_ = 0U;
        ebo_ = src.ebo_; src.ebo_ = 0U;
        index_count_  = src.index_count_;  src.index_count_  = 0;
        vertex_count_ = src.vertex_count_; src.vertex_count_ = 0;
    }
    return *this;
}

void TriMesh::update(TriMeshData const& data) {
    auto const vcount = static_cast<GLsizei>(data.vertices.size());
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    if (vcount == vertex_count_) {
        glBufferSubData(GL_ARRAY_BUFFER, 0,
            static_cast<GLsizeiptr>(data.vertices.size() * sizeof(TriVertex)),
            data.vertices.data());
    } else {
        glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(data.vertices.size() * sizeof(TriVertex)),
            data.vertices.data(), GL_DYNAMIC_DRAW);
        vertex_count_ = vcount;
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(data.indices.size() * sizeof(uint32_t)),
        data.indices.data(), GL_DYNAMIC_DRAW);
    index_count_ = static_cast<GLsizei>(data.indices.size());
    glBindVertexArray(0);
}

void TriMesh::draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}
