#pragma once
#include <GLES3/gl3.h>
#include "sphere_geometry.hpp"

struct SphereMesh {
    static SphereMesh create(const SphereGeometry& geo);

    SphereMesh() = default;
    ~SphereMesh();
    SphereMesh(const SphereMesh&) = delete;
    SphereMesh& operator=(const SphereMesh&) = delete;
    SphereMesh(SphereMesh&&) noexcept;
    SphereMesh& operator=(SphereMesh&&) noexcept;

    void draw() const;

private:
    GLuint vao_         = 0;
    GLuint vbo_         = 0;
    GLuint ebo_         = 0;
    GLsizei index_count_ = 0;
};
