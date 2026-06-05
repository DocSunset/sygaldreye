// Copyright 2025 Travis West
#include "chladni.hpp"
#include <cmath>

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

} // namespace

Chladni Chladni::create(ChladniParams const& p) {
    Chladni c;
    c.params_ = p;
    int n = p.grid_n;
    int nfaces = (n - 1) * (n - 1) * 2;
    c.data_.vertices.resize(static_cast<size_t>(nfaces * 3));
    c.data_.indices.resize(static_cast<size_t>(nfaces * 3));
    for (size_t i = 0; i < c.data_.indices.size(); ++i)
        c.data_.indices[i] = static_cast<uint32_t>(i);
    c.mesh_ = TriMesh::create(c.data_);

    auto prog = GlProgram::build(VERT, FRAG);
    if (prog) {
        c.prog_    = std::make_unique<GlProgram>(std::move(*prog));
        c.mvp_loc_ = c.prog_->uniform_location("uMVP");
    }
    return c;
}

void Chladni::update(float time_s) {
    auto& p = params_;
    int n = p.grid_n;
    float pi = static_cast<float>(M_PI);

    // Slowly morph between mode pairs
    float morph = std::fmod(time_s / 4.0f, 1.0f); // 0..1 cycle every 4s
    // Sequence: (3,4)->(4,5)->(2,3)->(5,2)->(3,4)...
    int pairs[5][2] = {{3,4},{4,5},{2,3},{5,2},{3,4}};
    int phase_idx = static_cast<int>(time_s / 4.0f) % 4;
    float m0 = static_cast<float>(pairs[phase_idx][0]);
    float m1 = static_cast<float>(pairs[phase_idx+1][0]);
    float n0 = static_cast<float>(pairs[phase_idx][1]);
    float n1 = static_cast<float>(pairs[phase_idx+1][1]);
    float mm = m0 + morph * (m1 - m0);
    float nn = n0 + morph * (n1 - n0);

    float cos_wt = std::cos(p.omega * time_s);

    // Precompute grid z values
    std::vector<float> grid(static_cast<size_t>(n * n));
    for (int gy = 0; gy < n; ++gy) {
        for (int gx = 0; gx < n; ++gx) {
            float x = static_cast<float>(gx) / static_cast<float>(n - 1);
            float y = static_cast<float>(gy) / static_cast<float>(n - 1);
            float z = (std::sin(mm * pi * x) * std::sin(nn * pi * y)
                     + std::sin(nn * pi * x) * std::sin(mm * pi * y)) * cos_wt;
            grid[static_cast<size_t>(gy * n + gx)] = z;
        }
    }

    int vi = 0;
    Eigen::Vector3f normal = {0.0f, 0.0f, 1.0f};
    for (int gy = 0; gy < n - 1; ++gy) {
        for (int gx = 0; gx < n - 1; ++gx) {
            auto emit = [&](int ax, int ay, int bx, int by, int cx, int cy) {
                int coords[3][2] = {{ax,ay},{bx,by},{cx,cy}};
                for (auto& co : coords) {
                    float x = static_cast<float>(co[0]) / static_cast<float>(n - 1);
                    float y = static_cast<float>(co[1]) / static_cast<float>(n - 1);
                    float z = grid[static_cast<size_t>(co[1] * n + co[0])];
                    float absz = std::abs(z);
                    // Map absz: near 0 = node_color (sand), large = anti_color (dark)
                    float t = std::min(absz * 1.5f, 1.0f);
                    Eigen::Vector4f col = p.node_color * (1.0f - t) + p.anti_color * t;
                    data_.vertices[static_cast<size_t>(vi++)] = {{x*2.0f-1.0f, y*2.0f-1.0f, 0.0f}, normal, col};
                }
            };
            emit(gx,gy, gx+1,gy, gx,gy+1);
            emit(gx+1,gy, gx+1,gy+1, gx,gy+1);
        }
    }
    mesh_.update(data_);
}

void Chladni::draw(Eigen::Matrix4f const& mvp) const {
    if (!prog_) return;
    prog_->use();
    GlProgram::uniform(mvp_loc_, mvp);
    mesh_.draw();
}
