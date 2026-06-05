// Copyright 2025 Travis West
#include "water_surface.hpp"
#include "log.hpp"
#include <Eigen/Geometry>
#include <cmath>

#define TAG "water_surface"
#define LOGE(...) LOG_E(TAG, __VA_ARGS__)

namespace {
// Vertex layout matches TriMesh: loc 0=position(vec3), loc 1=normal(vec3), loc 2=color(vec4).
constexpr const char* const VERT = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
flat out vec4 vColor;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

constexpr const char* const FRAG = R"(#version 300 es
precision mediump float;
flat in vec4 vColor;
out vec4 fragColor;
void main() { fragColor = vColor; }
)";

constexpr float F1 = 0.3f, S1 = 1.0f;
constexpr float F2 = 0.5f, S2 = 0.8f;

static TriMeshData build_mesh(WaterParams const& p) {
    int const nf = (p.grid_w - 1) * (p.grid_h - 1) * 2;
    TriMeshData d;
    d.vertices.resize(static_cast<size_t>(nf * 3));
    d.indices.resize(static_cast<size_t>(nf * 3));
    int vi = 0;
    for (int z = 0; z < p.grid_h - 1; ++z) {
        for (int x = 0; x < p.grid_w - 1; ++x) {
            int i00 = z * p.grid_w + x;
            auto emit_face = [&](int a, int b, int c) {
                auto idx_to_pos = [&](int idx) -> Eigen::Vector3f {
                    return { static_cast<float>(idx % p.grid_w) * p.cell_size,
                             0.0f,
                             static_cast<float>(idx / p.grid_w) * p.cell_size };
                };
                Eigen::Vector3f pa = idx_to_pos(a), pb = idx_to_pos(b), pc = idx_to_pos(c);
                Eigen::Vector3f n  = (pb - pa).cross(pc - pa).normalized();
                for (auto* v : {&pa, &pb, &pc}) {
                    d.vertices[static_cast<size_t>(vi)] = { *v, n, {0,0,0,1} };
                    d.indices[static_cast<size_t>(vi)]  = static_cast<uint32_t>(vi);
                    ++vi;
                }
            };
            emit_face(i00, i00 + 1, i00 + p.grid_w);
            emit_face(i00 + 1, i00 + p.grid_w + 1, i00 + p.grid_w);
        }
    }
    return d;
}
} // namespace

WaterSurface WaterSurface::create(WaterParams const& p) {
    WaterSurface w;
    w.params_ = p;
    w.data_   = build_mesh(p);
    w.mesh_   = TriMesh::create(w.data_);
    auto result = GlProgram::build(VERT, FRAG);
    if (!result) { LOGE("shader build failed"); return w; }
    w.prog_    = std::make_unique<GlProgram>(std::move(*result));
    w.mvp_loc_ = w.prog_->uniform_location("uMVP");
    return w;
}

void WaterSurface::update(float time_s) {
    auto& p   = params_;
    int vi = 0;
    for (int z = 0; z < p.grid_h - 1; ++z) {
        for (int x = 0; x < p.grid_w - 1; ++x) {
            int i00 = z * p.grid_w + x;
            auto height_at = [&](int idx) -> float {
                float fx = static_cast<float>(idx % p.grid_w) * p.cell_size;
                float fz = static_cast<float>(idx / p.grid_w) * p.cell_size;
                return p.wave_height * (std::sin(fx * F1 + time_s * S1 * p.wave_speed)
                                      + std::sin(fz * F2 + time_s * S2 * p.wave_speed)) * 0.5f;
            };
            auto emit_face = [&](int a, int b, int c) {
                auto idx_to_pos = [&](int idx) -> Eigen::Vector3f {
                    return { static_cast<float>(idx % p.grid_w) * p.cell_size,
                             height_at(idx),
                             static_cast<float>(idx / p.grid_w) * p.cell_size };
                };
                Eigen::Vector3f pa = idx_to_pos(a), pb = idx_to_pos(b), pc = idx_to_pos(c);
                Eigen::Vector3f n  = (pb - pa).cross(pc - pa).normalized();
                float avg_h = (pa.y() + pb.y() + pc.y()) / 3.0f;
                float t     = std::clamp((avg_h / p.wave_height + 1.0f) * 0.5f, 0.0f, 1.0f);
                Eigen::Vector4f col = p.shallow_color * (1.0f - t) + p.deep_color * t;
                for (auto* v : {&pa, &pb, &pc}) {
                    data_.vertices[static_cast<size_t>(vi)] = { *v, n, col };
                    ++vi;
                }
            };
            emit_face(i00, i00 + 1, i00 + p.grid_w);
            emit_face(i00 + 1, i00 + p.grid_w + 1, i00 + p.grid_w);
        }
    }
    mesh_.update(data_);
}

void WaterSurface::draw(Eigen::Matrix4f const& mvp) const {
    prog_->use();
    GlProgram::uniform(mvp_loc_, mvp);
    mesh_.draw();
}
