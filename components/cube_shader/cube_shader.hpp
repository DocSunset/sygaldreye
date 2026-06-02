#pragma once
#include <memory>
#include <Eigen/Core>
#include <GLES3/gl3.h>

struct GlProgram;

struct CubeShader {
    CubeShader();
    ~CubeShader();
    CubeShader(const CubeShader&) = delete;
    CubeShader& operator=(const CubeShader&) = delete;
    CubeShader(CubeShader&& src) noexcept;
    CubeShader& operator=(CubeShader&& src) noexcept;

    void init();
    void use() const;
    void set_mvp(const Eigen::Matrix4f& mvp) const;
    [[nodiscard]] GLint mvp_loc() const { return mvp_loc_; }

private:
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_ = -1;
};
