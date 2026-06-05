#pragma once
#include <memory>
#include <Eigen/Core>
#include <GLES3/gl3.h>

struct GlProgram;

struct FlatShader {
    FlatShader();
    ~FlatShader() = default;
    FlatShader(const FlatShader&) = delete;
    FlatShader& operator=(const FlatShader&) = delete;
    FlatShader(FlatShader&& src) noexcept;
    FlatShader& operator=(FlatShader&& src) noexcept;

    void create();
    void use() const;
    void set_mvp(const Eigen::Matrix4f& mvp) const;

private:
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_ = -1;
};
