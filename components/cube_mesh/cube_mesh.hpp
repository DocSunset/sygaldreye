#pragma once
#include <memory>
#include <Eigen/Core>
#include <GLES3/gl3.h>

struct GlProgram;

struct CubeMesh {
    CubeMesh();
    ~CubeMesh();

    CubeMesh(const CubeMesh&) = delete;
    CubeMesh& operator=(const CubeMesh&) = delete;

    CubeMesh(CubeMesh&& src) noexcept;
    CubeMesh& operator=(CubeMesh&& src) noexcept;

    void init();
    void begin_batch() const;
    static void end_batch();
    void draw(const Eigen::Matrix4f& mvp) const;
private:
    GLuint vao_ = 0U;
    GLuint vbo_ = 0U;
    GLuint ebo_ = 0U;
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_ = -1;
};
