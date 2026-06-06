// Copyright 2025 Travis West
#include "water_surface.hpp"
#include "log.hpp"
#include <Eigen/Geometry>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#define TAG "water_surface"

namespace {

constexpr int kMaxWaves = 32;

// Vertex shader: Gerstner waves with analytical normals, fully on GPU.
// aXZ = base grid position (x0, z0); uWaveA/B carry wave params as uniform arrays.
constexpr const char* VERT = R"(#version 300 es
precision highp float;
layout(location=0) in vec2 aXZ;
uniform mat4  uMVP;
uniform mat4  uModel;
uniform float uTime;
uniform vec4  uWaveA[32];  // (dir.x, dir.z, k, omega)
uniform vec4  uWaveB[32];  // (amplitude, steepness, phase, 0)

out highp vec3  vFragPos;
out float vHeight;

void main() {
    float x0 = aXZ.x;
    float z0 = aXZ.y;
    float dx = 0.0, dy = 0.0, dz = 0.0;

    for (int i = 0; i < 32; ++i) {
        float dirx = uWaveA[i].x;
        float dirz = uWaveA[i].y;
        float k    = uWaveA[i].z;
        float om   = uWaveA[i].w;
        float A    = uWaveB[i].x;
        float Q    = uWaveB[i].y;
        float ph0  = uWaveB[i].z;

        float ph = k * (dirx * x0 + dirz * z0) - om * uTime + ph0;
        float c  = cos(ph);

        dx += Q * A * dirx * c;
        dy += A * sin(ph);
        dz += Q * A * dirz * c;
    }

    vec3 local_pos = vec3(x0 + dx, dy, z0 + dz);
    vFragPos = (uModel * vec4(local_pos, 1.0)).xyz;
    vHeight  = dy;
    gl_Position = uMVP * vec4(local_pos, 1.0);
}
)";

constexpr const char* FRAG = R"(#version 300 es
precision mediump float;
in highp vec3  vFragPos;
in float vHeight;
uniform vec3  uViewPos;
uniform vec3  uSunDir;
uniform vec3  uSunColor;
uniform float uSunIntensity;
uniform vec3  uMatAmbient;
uniform vec3  uMatDiffuse;
uniform vec3  uMatSpecular;
uniform float uMatShininess;
uniform vec4  uShallowColor;
uniform vec4  uDeepColor;
uniform float uFoamThreshold;
uniform float uMaxAmp;
out vec4 fragColor;
void main() {
    float t    = clamp((vHeight / uMaxAmp + 1.0) * 0.5, 0.0, 1.0);
    vec3  base = mix(uDeepColor.rgb, uShallowColor.rgb, t);
    float foam = clamp((vHeight / uMaxAmp - uFoamThreshold)
                       / (1.0 - uFoamThreshold), 0.0, 1.0);
    base = mix(base * uMatDiffuse, vec3(1.0), foam);

    vec3  lightDir = normalize(-uSunDir);
    vec3  viewDir  = normalize(uViewPos - vFragPos);
    vec3  norm     = normalize(cross(dFdx(vFragPos), dFdy(vFragPos)));
    if (dot(norm, viewDir) < 0.0) norm = -norm;
    vec3  halfDir  = normalize(lightDir + viewDir);
    float diff     = max(dot(norm, lightDir), 0.0);
    float spec     = pow(max(dot(norm, halfDir), 0.0), uMatShininess);
    vec3  sun      = uSunColor * uSunIntensity;
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
    constexpr float base_wl    = 56.0f;
    constexpr float k_amp      = 0.016f;
    constexpr float base_steep = 0.60f;
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
        waves.push_back({{std::cos(angle), std::sin(angle)},
                         amplitude, wavelength, steepness, phase});
    }
    return waves;
}

void WaterSurface::upload_wave_params() {
    if (!prog_) return;
    prog_->use();
    constexpr float G = 9.81f;
    int n = std::min(static_cast<int>(params_.waves.size()), kMaxWaves);

    std::array<Eigen::Vector4f, kMaxWaves> wa{}, wb{};
    max_amp_ = 0.0f;
    for (int i = 0; i < n; ++i) {
        auto& wv = params_.waves[static_cast<size_t>(i)];
        Eigen::Vector2f d = wv.dir.normalized();
        float k  = 2.0f * float(M_PI) / wv.wavelength;
        float om = std::sqrt(G * k);
        wa[static_cast<size_t>(i)] = {d.x(), d.y(), k, om};
        wb[static_cast<size_t>(i)] = {wv.amplitude, wv.steepness, wv.phase, 0.0f};
        max_amp_ += wv.amplitude;
    }
    glUniform4fv(wave_a_loc_, kMaxWaves, wa[0].data());
    glUniform4fv(wave_b_loc_, kMaxWaves, wb[0].data());
}

WaterSurface WaterSurface::create(WaterParams const& p) {
    WaterSurface w;
    w.params_ = p;

    auto prog = GlProgram::build(VERT, FRAG);
    if (!prog) { LOG_E(TAG, "shader build failed"); return w; }
    w.prog_            = std::make_unique<GlProgram>(std::move(*prog));
    w.mvp_loc_         = w.prog_->uniform_location("uMVP");
    w.model_loc_       = w.prog_->uniform_location("uModel");
    w.view_pos_loc_    = w.prog_->uniform_location("uViewPos");
    w.time_loc_        = w.prog_->uniform_location("uTime");
    w.sun_dir_loc_     = w.prog_->uniform_location("uSunDir");
    w.sun_col_loc_     = w.prog_->uniform_location("uSunColor");
    w.sun_int_loc_     = w.prog_->uniform_location("uSunIntensity");
    w.mat_amb_loc_     = w.prog_->uniform_location("uMatAmbient");
    w.mat_dif_loc_     = w.prog_->uniform_location("uMatDiffuse");
    w.mat_spe_loc_     = w.prog_->uniform_location("uMatSpecular");
    w.mat_shi_loc_     = w.prog_->uniform_location("uMatShininess");
    w.shallow_loc_     = w.prog_->uniform_location("uShallowColor");
    w.deep_loc_        = w.prog_->uniform_location("uDeepColor");
    w.foam_thresh_loc_ = w.prog_->uniform_location("uFoamThreshold");
    w.max_amp_loc_     = w.prog_->uniform_location("uMaxAmp");
    w.wave_a_loc_      = w.prog_->uniform_location("uWaveA[0]");
    w.wave_b_loc_      = w.prog_->uniform_location("uWaveB[0]");
    w.upload_wave_params();

    int gw = p.grid_w, gh = p.grid_h;
    std::vector<float> verts;
    verts.reserve(static_cast<size_t>(gw * gh * 2));
    for (int z = 0; z < gh; ++z)
        for (int x = 0; x < gw; ++x) {
            verts.push_back(float(x) * p.cell_size);
            verts.push_back(float(z) * p.cell_size);
        }

    std::vector<uint32_t> indices;
    indices.reserve(static_cast<size_t>((gw - 1) * (gh - 1) * 6));
    for (int z = 0; z < gh - 1; ++z)
        for (int x = 0; x < gw - 1; ++x) {
            uint32_t i00 = static_cast<uint32_t>(z * gw + x);
            uint32_t i10 = i00 + 1u;
            uint32_t i01 = static_cast<uint32_t>((z + 1) * gw + x);
            uint32_t i11 = i01 + 1u;
            indices.insert(indices.end(), {i00, i10, i01, i10, i11, i01});
        }
    w.index_count_ = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &w.vao_);
    glGenBuffers(1, &w.vbo_);
    glGenBuffers(1, &w.ebo_);
    glBindVertexArray(w.vao_);
    glBindBuffer(GL_ARRAY_BUFFER, w.vbo_);
    glBufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
        verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, w.ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
        indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
    return w;
}

void WaterSurface::operator()(double time_s) {
    update(static_cast<float>(time_s));
    outputs.render.value = [this](const Eigen::Matrix4f& vp) {
        // Use identity model and zero view_pos as defaults; app can override via set_sun().
        draw(vp, Eigen::Matrix4f::Identity(), Eigen::Vector3f::Zero());
    };
}

void WaterSurface::draw(Eigen::Matrix4f const& mvp,
                        Eigen::Matrix4f const& model,
                        Eigen::Vector3f const& view_pos) const {
    if (!prog_ || vao_ == 0) return;
    prog_->use();
    GlProgram::uniform(mvp_loc_,   mvp);
    GlProgram::uniform(model_loc_, model);
    glUniform3fv(view_pos_loc_,    1, view_pos.data());
    glUniform1f (time_loc_,           time_s_);
    glUniform3fv(sun_dir_loc_,     1, params_.sun.direction.normalized().eval().data());
    glUniform3fv(sun_col_loc_,     1, params_.sun.color.data());
    glUniform1f (sun_int_loc_,        params_.sun.intensity);
    glUniform3fv(mat_amb_loc_,     1, params_.material.ambient.data());
    glUniform3fv(mat_dif_loc_,     1, params_.material.diffuse.data());
    glUniform3fv(mat_spe_loc_,     1, params_.material.specular.data());
    glUniform1f (mat_shi_loc_,        params_.material.shininess);
    glUniform4fv(shallow_loc_,     1, params_.shallow_color.data());
    glUniform4fv(deep_loc_,        1, params_.deep_color.data());
    glUniform1f (foam_thresh_loc_,    params_.foam_threshold);
    glUniform1f (max_amp_loc_,        max_amp_);
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

WaterSurface::~WaterSurface() {
    if (vao_ != 0) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_ != 0) { glDeleteBuffers(1, &vbo_); }
    if (ebo_ != 0) { glDeleteBuffers(1, &ebo_); }
}

WaterSurface::WaterSurface(WaterSurface&& o) noexcept
    : params_(std::move(o.params_)),
      time_s_(o.time_s_), max_amp_(o.max_amp_),
      vao_(o.vao_), vbo_(o.vbo_), ebo_(o.ebo_),
      index_count_(o.index_count_),
      prog_(std::move(o.prog_)),
      mvp_loc_(o.mvp_loc_), model_loc_(o.model_loc_),
      view_pos_loc_(o.view_pos_loc_), time_loc_(o.time_loc_),
      sun_dir_loc_(o.sun_dir_loc_), sun_col_loc_(o.sun_col_loc_),
      sun_int_loc_(o.sun_int_loc_),
      mat_amb_loc_(o.mat_amb_loc_), mat_dif_loc_(o.mat_dif_loc_),
      mat_spe_loc_(o.mat_spe_loc_), mat_shi_loc_(o.mat_shi_loc_),
      shallow_loc_(o.shallow_loc_), deep_loc_(o.deep_loc_),
      foam_thresh_loc_(o.foam_thresh_loc_), max_amp_loc_(o.max_amp_loc_),
      wave_a_loc_(o.wave_a_loc_), wave_b_loc_(o.wave_b_loc_) {
    o.vao_ = o.vbo_ = o.ebo_ = 0;
}

WaterSurface& WaterSurface::operator=(WaterSurface&& o) noexcept {
    if (this == &o) return *this;
    if (vao_ != 0) { glDeleteVertexArrays(1, &vao_); }
    if (vbo_ != 0) { glDeleteBuffers(1, &vbo_); }
    if (ebo_ != 0) { glDeleteBuffers(1, &ebo_); }
    params_          = std::move(o.params_);
    time_s_          = o.time_s_;
    max_amp_         = o.max_amp_;
    vao_             = o.vao_;   o.vao_ = 0;
    vbo_             = o.vbo_;   o.vbo_ = 0;
    ebo_             = o.ebo_;   o.ebo_ = 0;
    index_count_     = o.index_count_;
    prog_            = std::move(o.prog_);
    mvp_loc_         = o.mvp_loc_;
    model_loc_       = o.model_loc_;
    view_pos_loc_    = o.view_pos_loc_;
    time_loc_        = o.time_loc_;
    sun_dir_loc_     = o.sun_dir_loc_;
    sun_col_loc_     = o.sun_col_loc_;
    sun_int_loc_     = o.sun_int_loc_;
    mat_amb_loc_     = o.mat_amb_loc_;
    mat_dif_loc_     = o.mat_dif_loc_;
    mat_spe_loc_     = o.mat_spe_loc_;
    mat_shi_loc_     = o.mat_shi_loc_;
    shallow_loc_     = o.shallow_loc_;
    deep_loc_        = o.deep_loc_;
    foam_thresh_loc_ = o.foam_thresh_loc_;
    max_amp_loc_     = o.max_amp_loc_;
    wave_a_loc_      = o.wave_a_loc_;
    wave_b_loc_      = o.wave_b_loc_;
    return *this;
}
