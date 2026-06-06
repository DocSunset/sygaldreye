// Copyright 2025 Travis West
#include "reaction_diffusion.hpp"
#include "log.hpp"
#include <Eigen/Core>
#include <cmath>
#include <cstdint>

#define TAG "reaction_diffusion"

namespace {

constexpr const char* VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
out vec4 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

constexpr const char* FRAG = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() {
    fragColor = vColor;
}
)";

// Seed random squares with U=0.5, V=0.25 to kick off the reaction
void seed_grid(std::vector<float>& u, std::vector<float>& v, int w, int h) {
    // LCG for reproducible seeding
    uint32_t s = 0xDEADBEEFu;
    auto lcg = [&]() -> float {
        s = s * 1664525u + 1013904223u;
        return float(s >> 8) / float(1u << 24);
    };
    constexpr int n_seeds = 64;
    constexpr int sq_size = 6;
    for (int n = 0; n < n_seeds; ++n) {
        int cx = static_cast<int>(lcg() * float(w));
        int cy = static_cast<int>(lcg() * float(h));
        for (int dy = -sq_size; dy <= sq_size; ++dy) {
            for (int dx = -sq_size; dx <= sq_size; ++dx) {
                int x = (cx + dx + w) % w;
                int y = (cy + dy + h) % h;
                size_t idx = static_cast<size_t>(y * w + x);
                u[idx] = 0.5f;
                v[idx] = 0.25f;
            }
        }
    }
}

} // namespace

ReactionDiffusion ReactionDiffusion::create(RDParams const& p) {
    ReactionDiffusion rd;
    rd.params_ = p;

    size_t n = static_cast<size_t>(p.grid_w * p.grid_h);
    rd.u_.assign(n, 1.0f);
    rd.v_.assign(n, 0.0f);
    rd.u_next_.resize(n);
    rd.v_next_.resize(n);

    seed_grid(rd.u_, rd.v_, p.grid_w, p.grid_h);

    // Each cell = 2 triangles = 6 vertices (unindexed for simplicity)
    int n_tris = (p.grid_w - 1) * (p.grid_h - 1) * 2;
    rd.data_.vertices.resize(static_cast<size_t>(n_tris * 3));
    rd.data_.indices.resize(static_cast<size_t>(n_tris * 3));
    for (size_t i = 0; i < rd.data_.indices.size(); ++i)
        rd.data_.indices[i] = static_cast<uint32_t>(i);

    rd.mesh_ = TriMesh::create(rd.data_);

    auto prog = GlProgram::build(VERT, FRAG);
    if (!prog) { LOG_E(TAG, "shader build failed"); return rd; }
    rd.prog_    = std::make_unique<GlProgram>(std::move(*prog));
    rd.mvp_loc_ = rd.prog_->uniform_location("uMVP");
    return rd;
}

void ReactionDiffusion::update() {
    auto& p  = params_;
    int   w  = p.grid_w;
    int   h  = p.grid_h;
    float Du = p.Du;
    float Dv = p.Dv;
    float F  = p.F;
    float k  = p.k;
    float dt = p.dt;

    for (int step = 0; step < p.steps_per_frame; ++step) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // Wrap-around indices
                int xp = (x + 1) % w, xm = (x - 1 + w) % w;
                int yp = (y + 1) % h, ym = (y - 1 + h) % h;

                size_t c  = static_cast<size_t>(y  * w + x);
                size_t xn = static_cast<size_t>(y  * w + xp);
                size_t xs = static_cast<size_t>(y  * w + xm);
                size_t ye = static_cast<size_t>(yp * w + x);
                size_t yw = static_cast<size_t>(ym * w + x);

                float u = u_[c], v = v_[c];
                float lap_u = u_[xn] + u_[xs] + u_[ye] + u_[yw] - 4.0f * u;
                float lap_v = v_[xn] + v_[xs] + v_[ye] + v_[yw] - 4.0f * v;
                float uvv   = u * v * v;

                u_next_[c] = u + dt * (Du * lap_u - uvv + F * (1.0f - u));
                v_next_[c] = v + dt * (Dv * lap_v + uvv - (F + k) * v);
            }
        }
        std::swap(u_, u_next_);
        std::swap(v_, v_next_);
    }

    // Upload to mesh
    auto& p2   = params_;
    int   vi   = 0;
    float inv_w = 1.0f / float(p2.grid_w - 1);
    float inv_h = 1.0f / float(p2.grid_h - 1);
    Eigen::Vector3f normal = {0.0f, 1.0f, 0.0f};

    auto cell_color = [&](int cx, int cy) -> Eigen::Vector4f {
        float vval = std::clamp(v_[static_cast<size_t>(cy * w + cx)], 0.0f, 1.0f);
        return p2.color_a * (1.0f - vval) + p2.color_b * vval;
    };

    for (int y = 0; y < h - 1; ++y) {
        for (int x = 0; x < w - 1; ++x) {
            // Positions in [-1, 1] XZ plane at Y=0
            float x0 = float(x)   * inv_w * 2.0f - 1.0f;
            float x1 = float(x+1) * inv_w * 2.0f - 1.0f;
            float z0 = float(y)   * inv_h * 2.0f - 1.0f;
            float z1 = float(y+1) * inv_h * 2.0f - 1.0f;

            auto c00 = cell_color(x, y);
            auto c10 = cell_color(x+1, y);
            auto c01 = cell_color(x, y+1);
            auto c11 = cell_color(x+1, y+1);

            // Average color per triangle
            auto tri0_col = ((c00 + c10 + c01) / 3.0f).eval();
            auto tri1_col = ((c10 + c11 + c01) / 3.0f).eval();

            data_.vertices[static_cast<size_t>(vi++)] = {{x0, 0.0f, z0}, normal, tri0_col};
            data_.vertices[static_cast<size_t>(vi++)] = {{x1, 0.0f, z0}, normal, tri0_col};
            data_.vertices[static_cast<size_t>(vi++)] = {{x0, 0.0f, z1}, normal, tri0_col};

            data_.vertices[static_cast<size_t>(vi++)] = {{x1, 0.0f, z0}, normal, tri1_col};
            data_.vertices[static_cast<size_t>(vi++)] = {{x1, 0.0f, z1}, normal, tri1_col};
            data_.vertices[static_cast<size_t>(vi++)] = {{x0, 0.0f, z1}, normal, tri1_col};
        }
    }
    mesh_.update(data_);
}

void ReactionDiffusion::operator()(double /*time_s*/) {
    params_.Du              = inputs.Du.value;
    params_.Dv              = inputs.Dv.value;
    params_.F               = inputs.F.value;
    params_.k               = inputs.k.value;
    params_.dt              = inputs.dt.value;
    params_.steps_per_frame = static_cast<int>(inputs.steps_per_frame.value);
    update();
    outputs.render.value = [this](const Eigen::Matrix4f& vp) { draw(vp); };
}

void ReactionDiffusion::draw(Eigen::Matrix4f const& mvp) const {
    if (!prog_) return;
    prog_->use();
    GlProgram::uniform(mvp_loc_, mvp);
    mesh_.draw();
}
