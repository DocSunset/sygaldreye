// Copyright 2025 Travis West
#include "chladni.hpp"

#include <algorithm>
#include <cmath>
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

const Eigen::Vector4f kNodeColor{0.95f, 0.90f, 0.75f, 1.0f};  // sand/cream
const Eigen::Vector4f kAntiColor{0.05f, 0.08f, 0.18f, 1.0f};  // dark blue
}  // namespace

void Chladni::init() {
    data_       = std::make_shared<TriMeshData>();
    const int n = n_;
    data_->vertices.resize(static_cast<size_t>(n) * n);
    for (int gy = 0; gy < n; ++gy)
        for (int gx = 0; gx < n; ++gx) {
            float      x = float(gx) / float(n - 1) * 2.f - 1.f;
            float      y = float(gy) / float(n - 1) * 2.f - 1.f;
            TriVertex& v = data_->vertices[static_cast<size_t>(gy) * n + gx];
            v.position   = Eigen::Vector3f(x, y, 0.f);
            v.normal     = Eigen::Vector3f(0.f, 0.f, 1.f);
            v.color      = kNodeColor;
        }
    data_->indices.reserve(static_cast<size_t>(n - 1) * (n - 1) * 6);
    for (int gy = 0; gy < n - 1; ++gy)
        for (int gx = 0; gx < n - 1; ++gx) {
            uint32_t a = uint32_t(gy * n + gx), w = uint32_t(n);
            data_->indices.insert(data_->indices.end(), {a, a + 1, a + w, a + 1, a + w + 1, a + w});
        }
}

void Chladni::operator()(double time_s) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});
    if (!data_) init();

    const int   n     = n_;
    const float pi    = static_cast<float>(M_PI);
    const float t     = static_cast<float>(time_s);
    const float omega = endpoints.omega.get();

    // Morph between mode pairs, one cycle every 4s.
    const float morph       = std::fmod(t / 4.0f, 1.0f);
    const int   pairs[5][2] = {{3, 4}, {4, 5}, {2, 3}, {5, 2}, {3, 4}};
    const int   pi_         = static_cast<int>(t / 4.0f) % 4;
    const float mm = pairs[pi_][0] + morph * (pairs[pi_ + 1][0] - pairs[pi_][0]);
    const float nn = pairs[pi_][1] + morph * (pairs[pi_ + 1][1] - pairs[pi_][1]);
    const float cos_wt = std::cos(omega * t);

    for (int gy = 0; gy < n; ++gy)
        for (int gx = 0; gx < n; ++gx) {
            float x = float(gx) / float(n - 1);
            float y = float(gy) / float(n - 1);
            float z = (std::sin(mm * pi * x) * std::sin(nn * pi * y)
                       + std::sin(nn * pi * x) * std::sin(mm * pi * y))
                    * cos_wt;
            float ct = std::min(std::abs(z) * 1.5f, 1.0f);
            data_->vertices[static_cast<size_t>(gy) * n + gx].color =
                kNodeColor * (1.0f - ct) + kAntiColor * ct;
        }

    Mesh m;
    m.geometry = data_;
    m.mode     = Primitive::Triangles;
    m.dynamic  = true;  // colors change each frame (positions fixed)
    endpoints.mesh.value = std::move(m);

    Surface s;
    s.shader    = shader_;
    s.cull_back = false;  // flat plate, viewable from both sides
    endpoints.surface.value = std::move(s);
}
