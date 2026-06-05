// Copyright 2025 Travis West
#include "water_surface.hpp"
#include "log.hpp"
#include <Eigen/Geometry>
#include <cmath>
#include <cstdint>

#define TAG "water_surface"

namespace {
constexpr const char* const VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
uniform mat4 uModel;
flat out vec3 vNormal;
flat out vec4 vColor;
out vec3 vFragPos;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vFragPos = vec3(uModel * vec4(aPos, 1.0));
    vNormal = normalize(mat3(transpose(inverse(uModel))) * aNormal);
    vColor = aColor;
}
)";

constexpr const char* const FRAG = R"(#version 300 es
precision mediump float;
flat in vec3 vNormal;
flat in vec4 vColor;
in vec3 vFragPos;
uniform vec3  uViewPos;
uniform vec3  uSunDir;
uniform vec3  uSunColor;
uniform float uSunIntensity;
uniform vec3  uMatAmbient;
uniform vec3  uMatDiffuse;
uniform vec3  uMatSpecular;
uniform float uMatShininess;
out vec4 fragColor;
void main() {
    vec3  norm     = normalize(vNormal);
    vec3  lightDir = normalize(-uSunDir);
    vec3  viewDir  = normalize(uViewPos - vFragPos);
    vec3  halfDir  = normalize(lightDir + viewDir);
    float diff     = max(dot(norm, lightDir), 0.0);
    float spec     = pow(max(dot(norm, halfDir), 0.0), uMatShininess);
    vec3  sun      = uSunColor * uSunIntensity;
    float foam     = vColor.a;
    vec3  base     = mix(vColor.rgb * uMatDiffuse, vec3(1.0), foam);
    vec3  result   = base * uMatAmbient
                   + base * sun * diff
                   + uMatSpecular * sun * spec * (1.0 - foam * 0.8);
    fragColor = vec4(result, 1.0);
}
)";
} // namespace

std::vector<GerstnerWave> make_default_waves() {
    std::vector<GerstnerWave> waves;
    waves.reserve(32);
    constexpr float base_wl    = 56.0f;  // fundamental: 2 octaves below 14m
    constexpr float k_amp      = 0.016f; // A proportional to wavelength
    constexpr float base_steep = 0.60f;
    // LCG for reproducible pseudo-random wave parameters
    auto lcg = [](uint32_t& s) {
        s = s * 1664525u + 1013904223u;
        return float(s >> 8) / float(1u << 24);
    };
    uint32_t s_inh = 0xDEADBEEFu, s_ph = 0xCAFEBABEu, s_dir = 0x12345678u;
    for (int i = 1; i <= 32; ++i) {
        float fi         = float(i);
        float inharmonic = (lcg(s_inh) - 0.5f) * 0.18f;
        float wavelength = base_wl / (fi * (1.0f + inharmonic));
        float amplitude  = k_amp * wavelength;
        float steepness  = base_steep * std::exp(-fi * 0.025f);
        float phase      = lcg(s_ph)  * 2.0f * float(M_PI);
        float angle      = lcg(s_dir) * 2.0f * float(M_PI);
        waves.push_back({{std::cos(angle), std::sin(angle)}, amplitude, wavelength, steepness, phase});
    }
    return waves;
}

WaterSurface WaterSurface::create(WaterParams const& p) {
    WaterSurface w;
    w.params_ = p;
    int nf = (p.grid_w - 1) * (p.grid_h - 1) * 2;
    w.data_.vertices.resize(static_cast<size_t>(nf * 3));
    w.data_.indices.resize(static_cast<size_t>(nf * 3));
    for (size_t i = 0; i < w.data_.indices.size(); ++i)
        w.data_.indices[i] = static_cast<uint32_t>(i);
    w.mesh_         = TriMesh::create(w.data_);
    auto prog = GlProgram::build(VERT, FRAG);
    if (!prog) { LOG_E(TAG, "shader build failed"); return w; }
    w.prog_         = std::make_unique<GlProgram>(std::move(*prog));
    w.mvp_loc_      = w.prog_->uniform_location("uMVP");
    w.model_loc_    = w.prog_->uniform_location("uModel");
    w.view_pos_loc_ = w.prog_->uniform_location("uViewPos");
    w.sun_dir_loc_  = w.prog_->uniform_location("uSunDir");
    w.sun_col_loc_  = w.prog_->uniform_location("uSunColor");
    w.sun_int_loc_  = w.prog_->uniform_location("uSunIntensity");
    w.mat_amb_loc_  = w.prog_->uniform_location("uMatAmbient");
    w.mat_dif_loc_  = w.prog_->uniform_location("uMatDiffuse");
    w.mat_spe_loc_  = w.prog_->uniform_location("uMatSpecular");
    w.mat_shi_loc_  = w.prog_->uniform_location("uMatShininess");
    return w;
}

void WaterSurface::update(float time_s) {
    auto& p = params_;
    struct WC { Eigen::Vector2f d; float k, omega, a, q, phase; };
    std::vector<WC> wcs;
    wcs.reserve(p.waves.size());
    float max_amp = 0.0f;
    constexpr float G = 9.81f;
    for (auto& wv : p.waves) {
        float k  = 2.0f * float(M_PI) / wv.wavelength;
        wcs.push_back({wv.dir.normalized(), k, k * std::sqrt(G / k),
                       wv.amplitude, wv.steepness, wv.phase});
        max_amp += wv.amplitude;
    }

    // Precompute all grid positions once — avoids recomputing shared corners 6× each
    std::vector<Eigen::Vector3f> grid(static_cast<size_t>(p.grid_w * p.grid_h));
    for (int gz = 0; gz < p.grid_h; ++gz) {
        for (int gx = 0; gx < p.grid_w; ++gx) {
            float x0 = float(gx) * p.cell_size, z0 = float(gz) * p.cell_size;
            Eigen::Vector3f pos = {x0, 0.0f, z0};
            for (auto& w : wcs) {
                float ph = w.k * (w.d.x() * x0 + w.d.y() * z0) - w.omega * time_s + w.phase;
                pos.x() += w.q * w.a * w.d.x() * std::cos(ph);
                pos.y() += w.a * std::sin(ph);
                pos.z() += w.q * w.a * w.d.y() * std::cos(ph);
            }
            grid[static_cast<size_t>(gz * p.grid_w + gx)] = pos;
        }
    }

    int vi = 0;
    for (int z = 0; z < p.grid_h - 1; ++z) {
        for (int x = 0; x < p.grid_w - 1; ++x) {
            auto emit = [&](int ax, int az, int bx, int bz, int cx, int cz) {
                auto& pa = grid[static_cast<size_t>(az * p.grid_w + ax)];
                auto& pb = grid[static_cast<size_t>(bz * p.grid_w + bx)];
                auto& pc = grid[static_cast<size_t>(cz * p.grid_w + cx)];
                Eigen::Vector3f fn = (pc - pa).cross(pb - pa).normalized();
                float avg_y = (pa.y() + pb.y() + pc.y()) / 3.0f;
                float t_col = std::clamp((avg_y / max_amp + 1.0f) * 0.5f, 0.0f, 1.0f);
                Eigen::Vector4f col = p.deep_color * (1.0f - t_col) + p.shallow_color * t_col;
                col.w() = std::clamp((avg_y / max_amp - p.foam_threshold)
                                     / (1.0f - p.foam_threshold), 0.0f, 1.0f);
                for (auto& pv : {pa, pb, pc})
                    data_.vertices[static_cast<size_t>(vi++)] = {pv, fn, col};
            };
            emit(x,z,  x+1,z,   x,z+1);
            emit(x+1,z, x+1,z+1, x,z+1);
        }
    }
    mesh_.update(data_);
}

void WaterSurface::draw(Eigen::Matrix4f const& mvp,
                        Eigen::Matrix4f const& model,
                        Eigen::Vector3f const& view_pos) const {
    prog_->use();
    GlProgram::uniform(mvp_loc_,   mvp);
    GlProgram::uniform(model_loc_, model);
    glUniform3fv(view_pos_loc_, 1, view_pos.data());
    Eigen::Vector3f sd = params_.sun.direction.normalized();
    glUniform3fv(sun_dir_loc_, 1, sd.data());
    glUniform3fv(sun_col_loc_, 1, params_.sun.color.data());
    glUniform1f (sun_int_loc_,    params_.sun.intensity);
    glUniform3fv(mat_amb_loc_, 1, params_.material.ambient.data());
    glUniform3fv(mat_dif_loc_, 1, params_.material.diffuse.data());
    glUniform3fv(mat_spe_loc_, 1, params_.material.specular.data());
    glUniform1f (mat_shi_loc_,    params_.material.shininess);
    mesh_.draw();
}
