#pragma once
#include <memory>
#include <Eigen/Core>
#include <GLES3/gl3.h>

struct GlProgram;

struct WindShader {
    WindShader();
    ~WindShader() = default;
    WindShader(const WindShader&) = delete;
    WindShader& operator=(const WindShader&) = delete;
    WindShader(WindShader&& src) noexcept;
    WindShader& operator=(WindShader&& src) noexcept;

    void create();
    void use() const;
    void set_mvp(const Eigen::Matrix4f& mvp) const;
    void set_time(float t) const;
    void set_wind(Eigen::Vector3f dir, float strength) const;

private:
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_      = -1;
    GLint time_loc_     = -1;
    GLint wind_dir_loc_ = -1;
    GLint wind_str_loc_ = -1;
};
