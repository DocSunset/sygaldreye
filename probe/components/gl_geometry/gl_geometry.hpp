// Copyright 2025 Travis West
#pragma once
#include <GLES3/gl3.h>
#include <cstdint>
#include <span>
#include <vector>

struct AttribDesc {
    GLuint    index;   // layout location
    GLint     size;    // components (1–4)
    GLsizei   stride;  // total stride in bytes (same for all attribs in one VBO)
    uintptr_t offset;  // byte offset of this attrib within stride
};

class GlGeometry {
public:
    static GlGeometry create(std::span<const float>    verts,
                             std::span<const uint32_t> indices,
                             std::vector<AttribDesc>   layout,
                             GLenum                    vbo_usage = GL_STATIC_DRAW);

    void update_verts(std::span<const float> verts);
    void draw_arrays(GLenum mode, GLsizei count) const;
    void draw_elements(GLenum mode, GLsizei count) const;

    // VAO handle, so a caller (render_region) can attach per-instance
    // attribute buffers (divisor 1) for instanced draws and tear them down.
    [[nodiscard]] GLuint vao() const { return vao_; }

    GlGeometry() = default;
    ~GlGeometry();
    GlGeometry(const GlGeometry&) = delete;
    GlGeometry& operator=(const GlGeometry&) = delete;
    GlGeometry(GlGeometry&&) noexcept;
    GlGeometry& operator=(GlGeometry&&) noexcept;

private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
};
