// Copyright 2025 Travis West
#include "water_surface.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "tri_mesh.hpp"

namespace {

constexpr int kGrid = 64;

// Vertex shader: Gerstner waves, analytical, fully on GPU. Reads aPos.xz (the
// flat grid); displaces by the wave sum at uTime. uMVP/uTime injected by
// render_region. Geometry is world-space (no model matrix).
constexpr const char* VERT = R"(#version 300 es
precision highp float;
layout(location=0) in vec3 aPos;
uniform mat4  uMVP;
uniform float uTime;
uniform vec4  uWaveA[32];  // (dir.x, dir.z, k, omega)
uniform vec4  uWaveB[32];  // (amplitude, steepness, phase, 0)
out highp vec3 vFragPos;
out float vHeight;
void main() {
    float x0 = aPos.x;
    float z0 = aPos.z;
    float dx = 0.0, dy = 0.0, dz = 0.0;
    for (int i = 0; i < 32; ++i) {
        float dirx = uWaveA[i].x, dirz = uWaveA[i].y, k = uWaveA[i].z, om = uWaveA[i].w;
        float A = uWaveB[i].x, Q = uWaveB[i].y, ph0 = uWaveB[i].z;
        float ph = k * (dirx * x0 + dirz * z0) - om * uTime + ph0;
        float c  = cos(ph);
        dx += Q * A * dirx * c;
        dy += A * sin(ph);
        dz += Q * A * dirz * c;
    }
    vec3 p = vec3(x0 + dx, dy, z0 + dz);
    vFragPos = p;
    vHeight  = dy;
    gl_Position = uMVP * vec4(p, 1.0);
}
)";

constexpr const char* FRAG = R"(#version 300 es
precision mediump float;
in highp vec3 vFragPos;
in float vHeight;
uniform vec3  uViewPos;
uniform vec3  uSunDir;
uniform vec3  uSunColor;
uniform float uSunIntensity;
uniform float uFoamThreshold;
uniform float uMaxAmp;
out vec4 fragColor;
const vec3  MAT_AMBIENT  = vec3(0.04, 0.10, 0.16);
const vec3  MAT_DIFFUSE  = vec3(0.55, 0.70, 0.80);
const vec3  MAT_SPECULAR = vec3(0.90, 0.95, 1.00);
const float MAT_SHININESS = 80.0;
const vec3  SHALLOW = vec3(0.15, 0.55, 0.75);
const vec3  DEEP    = vec3(0.03, 0.18, 0.42);
vec2 blinn_phong(vec3 N, vec3 L, vec3 V, float shininess) {
    vec3 H = normalize(L + V);
    return vec2(max(dot(N, L), 0.0), pow(max(dot(N, H), 0.0), shininess));
}
void main() {
    float t    = clamp((vHeight / uMaxAmp + 1.0) * 0.5, 0.0, 1.0);
    vec3  base = mix(DEEP, SHALLOW, t);
    float foam = clamp((vHeight / uMaxAmp - uFoamThreshold) / (1.0 - uFoamThreshold), 0.0, 1.0);
    base = mix(base * MAT_DIFFUSE, vec3(1.0), foam);

    vec3 lightDir = normalize(-uSunDir);
    vec3 viewDir  = normalize(uViewPos - vFragPos);
    vec3 norm     = normalize(cross(dFdx(vFragPos), dFdy(vFragPos)));
    if (dot(norm, viewDir) < 0.0) norm = -norm;
    vec2  ds   = blinn_phong(norm, lightDir, viewDir, MAT_SHININESS);
    vec3  sun  = uSunColor * uSunIntensity;
    vec3  result = base * MAT_AMBIENT + base * sun * ds.x
                 + MAT_SPECULAR * sun * ds.y * (1.0 - foam * 0.8);
    fragColor = vec4(result, 1.0);
}
)";

// Flat grid centered at the origin, y=0, as TriVertex (only position used).
MeshPtr make_grid(float cell) {
    auto        m  = std::make_shared<TriMeshData>();
    const float ox = (kGrid - 1) * cell * 0.5f;
    m->vertices.reserve(static_cast<size_t>(kGrid * kGrid));
    for (int z = 0; z < kGrid; ++z)
        for (int x = 0; x < kGrid; ++x) {
            TriVertex v;
            v.position = Eigen::Vector3f(float(x) * cell - ox, 0.f, float(z) * cell - ox);
            v.normal   = Eigen::Vector3f(0.f, 1.f, 0.f);
            v.color    = Eigen::Vector4f(1.f, 1.f, 1.f, 1.f);
            m->vertices.push_back(v);
        }
    m->indices.reserve(static_cast<size_t>((kGrid - 1) * (kGrid - 1) * 6));
    for (int z = 0; z < kGrid - 1; ++z)
        for (int x = 0; x < kGrid - 1; ++x) {
            uint32_t i00 = static_cast<uint32_t>(z * kGrid + x);
            uint32_t i10 = i00 + 1u;
            uint32_t i01 = static_cast<uint32_t>((z + 1) * kGrid + x);
            uint32_t i11 = i01 + 1u;
            m->indices.insert(m->indices.end(), {i00, i10, i01, i10, i11, i01});
        }
    return m;
}

}  // namespace

std::vector<GerstnerWave> make_default_waves() { return make_waves(56.f, 0.016f, 0.6f); }

std::vector<GerstnerWave> make_waves(float base_wl, float k_amp, float base_steep) {
    std::vector<GerstnerWave> waves;
    waves.reserve(32);
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
        float phase      = lcg(s_ph) * 2.0f * float(M_PI);
        float angle      = lcg(s_dir) * 2.0f * float(M_PI);
        waves.push_back({{std::cos(angle), std::sin(angle)}, amplitude, wavelength, steepness, phase});
    }
    return waves;
}

void WaterSurface::rebuild_waves() {
    auto            waves = make_waves(spec_wl_, spec_amp_, spec_steep_);
    constexpr float G     = 9.81f;
    wave_uniforms_.clear();
    max_amp_ = 0.0f;
    int n    = std::min(32, static_cast<int>(waves.size()));
    for (int i = 0; i < n; ++i) {
        auto&           wv = waves[static_cast<size_t>(i)];
        Eigen::Vector2f d  = wv.dir.normalized();
        float           k  = 2.0f * float(M_PI) / wv.wavelength;
        float           om = std::sqrt(G * k);
        char            na[16], nb[16];
        std::snprintf(na, sizeof(na), "uWaveA[%d]", i);
        std::snprintf(nb, sizeof(nb), "uWaveB[%d]", i);
        wave_uniforms_.push_back({na, Eigen::Vector4f(d.x(), d.y(), k, om)});
        wave_uniforms_.push_back({nb, Eigen::Vector4f(wv.amplitude, wv.steepness, wv.phase, 0.f)});
        max_amp_ += wv.amplitude;
    }
}

void WaterSurface::operator()(double /*time_s*/) {
    const float cs = endpoints.cell_size.get();
    if (!grid_ || cs != cell_) {
        cell_ = cs;
        grid_ = make_grid(cell_);
    }
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{VERT, FRAG});
    if (endpoints.wavelength.get() != spec_wl_ || endpoints.choppiness.get() != spec_steep_
        || endpoints.amplitude.get() != spec_amp_) {
        spec_wl_    = endpoints.wavelength.get();
        spec_steep_ = endpoints.choppiness.get();
        spec_amp_   = endpoints.amplitude.get();
        rebuild_waves();
    }

    Mesh m;
    m.geometry = grid_;
    m.mode     = Primitive::Triangles;
    endpoints.mesh.value = std::move(m);

    const Eigen::Vector3f sdir =
        endpoints.sun_dir.src ? *endpoints.sun_dir.src : Eigen::Vector3f(-0.4f, -0.9f, -0.2f);
    const Eigen::Vector3f scol =
        endpoints.sun_color.src ? *endpoints.sun_color.src : Eigen::Vector3f(1.0f, 0.95f, 0.85f);

    Surface s;
    s.shader    = shader_;
    s.cull_back = false;  // wavy surface viewed from above/below — two-sided
    s.uniforms.push_back({"uSunDir", sdir.normalized().eval()});
    s.uniforms.push_back({"uSunColor", scol});
    s.uniforms.push_back({"uSunIntensity", endpoints.sun_intensity.get()});
    s.uniforms.push_back({"uFoamThreshold", endpoints.foam_threshold.get()});
    s.uniforms.push_back({"uMaxAmp", max_amp_});
    for (auto const& u : wave_uniforms_) s.uniforms.push_back(u);
    endpoints.surface.value = std::move(s);
}
