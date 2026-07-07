#pragma once
#include <GLES3/gl3.h>

struct CubeGeometry {
    CubeGeometry();
    ~CubeGeometry();
    CubeGeometry(const CubeGeometry&) = delete;
    CubeGeometry& operator=(const CubeGeometry&) = delete;
    CubeGeometry(CubeGeometry&& src) noexcept;
    CubeGeometry& operator=(CubeGeometry&& src) noexcept;

    void init();
    void bind() const;
    static void unbind();
    static void draw_elements();

private:
    GLuint vao_ = 0U;
    GLuint vbo_ = 0U;
    GLuint ebo_ = 0U;
};
