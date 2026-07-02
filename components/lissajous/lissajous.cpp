// Copyright 2025 Travis West
#include "lissajous.hpp"

#include <cmath>
#include <memory>

#include "tri_mesh.hpp"

namespace {

Eigen::Vector3f hsv2rgb(float h, float s, float v) {
    float h6 = h * 6.0f;
    int   i  = static_cast<int>(h6) % 6;
    float f  = h6 - std::floor(h6);
    float p  = v * (1.0f - s);
    float q  = v * (1.0f - f * s);
    float t  = v * (1.0f - (1.0f - f) * s);
    switch (i) {
        case 0:  return {v, t, p};
        case 1:  return {q, v, p};
        case 2:  return {p, v, t};
        case 3:  return {p, q, v};
        case 4:  return {t, p, v};
        default: return {v, p, q};
    }
}

}  // namespace

#include "common_shaders.hpp"

void Lissajous::operator()(double time_s) {
    if (!shader_)
        shader_ = std::make_shared<ShaderData>(ShaderData{
            common_shaders::kUnlitVertexColorVert, common_shaders::kUnlitVertexColorFrag});

    const float fx    = endpoints.freq_x.get();
    const float fy    = endpoints.freq_y.get();
    const float fz0   = endpoints.freq_z.get();
    const float amp   = endpoints.amp.get();
    const int   n     = std::max(2, static_cast<int>(endpoints.samples.get()));
    const float phase = endpoints.phase_x.get()
                      + static_cast<float>(time_s) * (2.0f * static_cast<float>(M_PI) / 16.0f);
    const float fz    = fz0 + std::sin(static_cast<float>(time_s) * 0.1f) * 0.3f;

    if (!data_) data_ = std::make_shared<TriMeshData>();
    auto& data = data_;
    data->vertices.clear();
    data->vertices.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(n - 1) * 2.0f * static_cast<float>(M_PI);
        Eigen::Vector3f rgb = hsv2rgb(static_cast<float>(i) / static_cast<float>(n), 0.85f, 1.0f);
        TriVertex v;
        v.position = Eigen::Vector3f(amp * std::sin(fx * t + phase), amp * std::sin(fy * t),
                                     amp * 0.3f * std::sin(fz * t));
        v.normal   = Eigen::Vector3f::Zero();
        v.color    = Eigen::Vector4f(rgb.x(), rgb.y(), rgb.z(), 1.0f);
        data->vertices.push_back(v);
    }

    data->touch();

    Mesh m;
    m.geometry = data_;  // no indices → draw_arrays(GL_LINE_STRIP)
    m.mode     = Primitive::LineStrip;
    m.dynamic  = true;
    endpoints.mesh.value = std::move(m);

    Surface s;
    s.shader    = shader_;
    s.cull_back = false;
    endpoints.surface.value = std::move(s);
}
