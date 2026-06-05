// Copyright 2024 Travis West
#include "lit_shader.hpp"
#include "gl_program.hpp"
#include "light.hpp"
#include "material.hpp"
#include <android/log.h>
#include <cstdio>

#define TAG "lit_shader"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace {
constexpr const char* const VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 vNormal;
out vec3 vFragPos;
out vec2 vUV;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vFragPos = vec3(uModel * vec4(aPos, 1.0));
    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    vNormal = normalize(normalMatrix * aNormal);
    vUV = aUV;
}
)";

constexpr const char* const FRAG = R"(#version 300 es
precision mediump float;
in vec3 vNormal;
in vec3 vFragPos;
in vec2 vUV;
uniform vec3 uViewPos;
struct LightUniforms {
    vec3 direction;
    vec3 color;
    float intensity;
};
const int MAX_LIGHTS = 8;
uniform LightUniforms uLights[MAX_LIGHTS];
uniform int uLightCount;
struct MaterialUniforms {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};
uniform MaterialUniforms uMaterial;
out vec4 fragColor;
void main() {
    vec3 norm = normalize(vNormal);
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 result = vec3(0.0);
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (i >= uLightCount) break;
        vec3 lightDir = normalize(-uLights[i].direction);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfDir), 0.0), uMaterial.shininess);
        vec3 lightCol = uLights[i].color * uLights[i].intensity;
        result += uMaterial.ambient * lightCol;
        result += uMaterial.diffuse * lightCol * diff;
        result += uMaterial.specular * lightCol * spec;
    }
    fragColor = vec4(result, 1.0);
}
)";
} // namespace

LitShader::LitShader() {
    light_dir_locs_.fill(-1);
    light_color_locs_.fill(-1);
    light_intensity_locs_.fill(-1);
}

LitShader::~LitShader() = default;
LitShader::LitShader(LitShader&&) noexcept = default;
LitShader& LitShader::operator=(LitShader&&) noexcept = default;

void LitShader::init() {
    auto result = GlProgram::build(VERT, FRAG);
    if (!result) { LOGE("shader build failed"); return; }
    prog_ = std::make_unique<GlProgram>(std::move(*result));
    mvp_loc_          = prog_->uniform_location("uMVP");
    model_loc_        = prog_->uniform_location("uModel");
    view_pos_loc_     = prog_->uniform_location("uViewPos");
    light_count_loc_  = prog_->uniform_location("uLightCount");
    mat_ambient_loc_  = prog_->uniform_location("uMaterial.ambient");
    mat_diffuse_loc_  = prog_->uniform_location("uMaterial.diffuse");
    mat_specular_loc_ = prog_->uniform_location("uMaterial.specular");
    mat_shininess_loc_= prog_->uniform_location("uMaterial.shininess");
    for (int idx = 0; idx < MAX_LIGHTS; ++idx) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "uLights[%d].direction", idx);
        light_dir_locs_[idx] = prog_->uniform_location(buf);
        std::snprintf(buf, sizeof(buf), "uLights[%d].color", idx);
        light_color_locs_[idx] = prog_->uniform_location(buf);
        std::snprintf(buf, sizeof(buf), "uLights[%d].intensity", idx);
        light_intensity_locs_[idx] = prog_->uniform_location(buf);
    }
}

void LitShader::use() const { prog_->use(); }

void LitShader::set_mvp(const Eigen::Matrix4f& mvp) const {
    GlProgram::uniform(mvp_loc_, mvp);
}

void LitShader::set_model(const Eigen::Matrix4f& model) const {
    GlProgram::uniform(model_loc_, model);
}

void LitShader::set_view_pos(const Eigen::Vector3f& pos) const {
    glUniform3fv(view_pos_loc_, 1, pos.data());
}

void LitShader::set_lights(std::span<const Light> lights) const {
    const int count = static_cast<int>(std::min(lights.size(),
                                                 static_cast<size_t>(MAX_LIGHTS)));
    for (int idx = 0; idx < count; ++idx) {
        glUniform3fv(light_dir_locs_[idx],       1, lights[idx].direction.data());
        glUniform3fv(light_color_locs_[idx],     1, lights[idx].color.data());
        glUniform1f(light_intensity_locs_[idx],  lights[idx].intensity);
    }
    glUniform1i(light_count_loc_, count);
}

void LitShader::set_material(const Material& mat) const {
    glUniform3fv(mat_ambient_loc_,  1, mat.ambient.data());
    glUniform3fv(mat_diffuse_loc_,  1, mat.diffuse.data());
    glUniform3fv(mat_specular_loc_, 1, mat.specular.data());
    glUniform1f(mat_shininess_loc_, mat.shininess);
}
