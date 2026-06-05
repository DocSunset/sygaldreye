#pragma once
#include <Eigen/Core>
#include <GLES3/gl3.h>
#include <array>
#include <memory>
#include <span>

struct GlProgram;
struct Light;
struct Material;

struct LitShader {
    LitShader();
    ~LitShader() = default;
    LitShader(const LitShader&) = delete;
    LitShader& operator=(const LitShader&) = delete;
    LitShader(LitShader&&) noexcept;
    LitShader& operator=(LitShader&&) noexcept;

    void init();
    void use() const;
    void set_mvp(const Eigen::Matrix4f& mvp) const;
    void set_model(const Eigen::Matrix4f& model) const;
    void set_view_pos(const Eigen::Vector3f& pos) const;
    void set_lights(std::span<const Light> lights) const;
    void set_material(const Material& mat) const;

private:
    static constexpr int MAX_LIGHTS = 8;
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_        = -1;
    GLint model_loc_      = -1;
    GLint view_pos_loc_   = -1;
    GLint light_count_loc_ = -1;
    GLint mat_ambient_loc_   = -1;
    GLint mat_diffuse_loc_   = -1;
    GLint mat_specular_loc_  = -1;
    GLint mat_shininess_loc_ = -1;
    std::array<GLint, MAX_LIGHTS> light_dir_locs_{};
    std::array<GLint, MAX_LIGHTS> light_color_locs_{};
    std::array<GLint, MAX_LIGHTS> light_intensity_locs_{};
};
