// Copyright 2025 Travis West
#include "gl_geometry.hpp"

GlGeometry GlGeometry::create(std::span<const float>    verts,
                               std::span<const uint32_t> indices,
                               std::vector<AttribDesc>   layout,
                               GLenum                    vbo_usage) {
    GlGeometry g;
    glGenVertexArrays(1, &g.vao_);
    glGenBuffers(1, &g.vbo_);
    glBindVertexArray(g.vao_);

    glBindBuffer(GL_ARRAY_BUFFER, g.vbo_);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(verts.size_bytes()),
        verts.empty() ? nullptr : verts.data(),
        vbo_usage);

    for (auto const& a : layout) {
        glEnableVertexAttribArray(a.index);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr)
        glVertexAttribPointer(a.index, a.size, GL_FLOAT, GL_FALSE, a.stride,
            reinterpret_cast<const void*>(a.offset));
    }

    if (!indices.empty()) {
        glGenBuffers(1, &g.ebo_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(indices.size_bytes()),
            indices.data(), GL_STATIC_DRAW);
    }

    glBindVertexArray(0);
    return g;
}

void GlGeometry::update_verts(std::span<const float> verts) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        static_cast<GLsizeiptr>(verts.size_bytes()),
        verts.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlGeometry::draw_arrays(GLenum mode, GLsizei count) const {
    glBindVertexArray(vao_);
    glDrawArrays(mode, 0, count);
    glBindVertexArray(0);
}

void GlGeometry::draw_elements(GLenum mode, GLsizei count) const {
    glBindVertexArray(vao_);
    glDrawElements(mode, count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

GlGeometry::~GlGeometry() {
    if (vao_ != 0U) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_ != 0U) { glDeleteBuffers(1, &vbo_); }
    if (ebo_ != 0U) { glDeleteBuffers(1, &ebo_); }
}

GlGeometry::GlGeometry(GlGeometry&& src) noexcept
    : vao_(src.vao_), vbo_(src.vbo_), ebo_(src.ebo_) {
    src.vao_ = 0U; src.vbo_ = 0U; src.ebo_ = 0U;
}

GlGeometry& GlGeometry::operator=(GlGeometry&& src) noexcept {
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
