#pragma once
#include <memory>
#include <Eigen/Core>
#include <GLES3/gl3.h>

struct GlProgram;

struct RgbaShader {
    RgbaShader();
    ~RgbaShader() = default;
    RgbaShader(const RgbaShader&) = delete;
    RgbaShader& operator=(const RgbaShader&) = delete;
    RgbaShader(RgbaShader&& src) noexcept;
    RgbaShader& operator=(RgbaShader&& src) noexcept;

    void create();
    void use() const;
    void set_mvp(const Eigen::Matrix4f& mvp) const;
    void set_color(const Eigen::Vector4f& color) const;

private:
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_   = -1;
    GLint color_loc_ = -1;
};
