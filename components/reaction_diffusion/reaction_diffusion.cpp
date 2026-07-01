// Copyright 2025 Travis West
#include "reaction_diffusion.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

namespace {

constexpr const char* kVert = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
out vec4 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() { fragColor = vColor; }
)";

void seed_grid(std::vector<float>& u, std::vector<float>& v, int w, int h) {
    uint32_t s   = 0xDEADBEEFu;
    auto     lcg = [&]() -> float {
        s = s * 1664525u + 1013904223u;
        return float(s >> 8) / float(1u << 24);
    };
    constexpr int n_seeds = 64, sq = 6;
    for (int n = 0; n < n_seeds; ++n) {
        int cx = static_cast<int>(lcg() * float(w));
        int cy = static_cast<int>(lcg() * float(h));
        for (int dy = -sq; dy <= sq; ++dy)
            for (int dx = -sq; dx <= sq; ++dx) {
                int    x   = (cx + dx + w) % w;
                int    y   = (cy + dy + h) % h;
                size_t idx = static_cast<size_t>(y * w + x);
                u[idx]     = 0.5f;
                v[idx]     = 0.25f;
            }
    }
}

}  // namespace

void ReactionDiffusion::init() {
    const int w = params_.grid_w, h = params_.grid_h;
    size_t    n = static_cast<size_t>(w * h);
    u_.assign(n, 1.0f);
    v_.assign(n, 0.0f);
    u_next_.resize(n);
    v_next_.resize(n);
    seed_grid(u_, v_, w, h);

    // Shared-vertex grid on the XZ plane in [-1,1]; color per vertex from V.
    data_ = std::make_shared<TriMeshData>();
    data_->vertices.resize(n);
    float inv_w = 1.0f / float(w - 1), inv_h = 1.0f / float(h - 1);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            TriVertex& vtx = data_->vertices[static_cast<size_t>(y * w + x)];
            vtx.position   = Eigen::Vector3f(float(x) * inv_w * 2.f - 1.f, 0.f,
                                             float(y) * inv_h * 2.f - 1.f);
            vtx.normal     = Eigen::Vector3f(0.f, 1.f, 0.f);
            vtx.color      = params_.color_a;
        }
    data_->indices.reserve(static_cast<size_t>(w - 1) * (h - 1) * 6);
    for (int y = 0; y < h - 1; ++y)
        for (int x = 0; x < w - 1; ++x) {
            uint32_t a = uint32_t(y * w + x), ww = uint32_t(w);
            data_->indices.insert(data_->indices.end(),
                                  {a, a + 1, a + ww, a + 1, a + ww + 1, a + ww});
        }
}

void ReactionDiffusion::step_sim() {
    const int w = params_.grid_w, h = params_.grid_h;
    const float Du = params_.Du, Dv = params_.Dv, F = params_.F, k = params_.k, dt = params_.dt;
    for (int step = 0; step < params_.steps_per_frame; ++step) {
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                int    xp = (x + 1) % w, xm = (x - 1 + w) % w;
                int    yp = (y + 1) % h, ym = (y - 1 + h) % h;
                size_t c     = static_cast<size_t>(y * w + x);
                float  u     = u_[c], v = v_[c];
                float  lap_u = u_[static_cast<size_t>(y * w + xp)] + u_[static_cast<size_t>(y * w + xm)]
                            + u_[static_cast<size_t>(yp * w + x)] + u_[static_cast<size_t>(ym * w + x)]
                            - 4.0f * u;
                float lap_v = v_[static_cast<size_t>(y * w + xp)] + v_[static_cast<size_t>(y * w + xm)]
                            + v_[static_cast<size_t>(yp * w + x)] + v_[static_cast<size_t>(ym * w + x)]
                            - 4.0f * v;
                float uvv  = u * v * v;
                u_next_[c] = u + dt * (Du * lap_u - uvv + F * (1.0f - u));
                v_next_[c] = v + dt * (Dv * lap_v + uvv - (F + k) * v);
            }
        std::swap(u_, u_next_);
        std::swap(v_, v_next_);
    }
}

void ReactionDiffusion::operator()(double /*time_s*/) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});
    if (!data_) init();

    params_.Du              = endpoints.Du.get();
    params_.Dv              = endpoints.Dv.get();
    params_.F               = endpoints.F.get();
    params_.k               = endpoints.k.get();
    params_.dt              = endpoints.dt.get();
    params_.steps_per_frame = static_cast<int>(endpoints.steps_per_frame.get());
    step_sim();

    const int w = params_.grid_w, h = params_.grid_h;
    for (int i = 0; i < w * h; ++i) {
        float vv = std::clamp(v_[static_cast<size_t>(i)], 0.0f, 1.0f);
        data_->vertices[static_cast<size_t>(i)].color =
            params_.color_a * (1.0f - vv) + params_.color_b * vv;
    }

    Mesh m;
    m.geometry = data_;
    m.mode     = Primitive::Triangles;
    m.dynamic  = true;  // colors change each frame (positions fixed)
    endpoints.mesh.value = std::move(m);

    Surface s;
    s.shader    = shader_;
    s.cull_back = false;
    endpoints.surface.value = std::move(s);
}
