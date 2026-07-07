// Copyright 2025 Travis West
#include "text_label.hpp"

#include <memory>
#include <string>
#include <vector>

#include <Eigen/Core>

#include "glyph_layout.hpp"
#include "tri_mesh.hpp"

namespace {
// MSDF text: aPos in world, UV carried in aNormal.xy (TriVertex has no UV).
constexpr const char* kVert = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uMVP;
out vec2 vUV;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    vUV = aNormal.xy;
}
)";
constexpr const char* kFrag = R"(#version 300 es
precision mediump float;
in vec2 vUV;
uniform sampler2D uAtlas;
uniform float uRange;
uniform vec4 uColor;
out vec4 fragColor;
float median(float r, float g, float b) { return max(min(r, g), min(max(r, g), b)); }
void main() {
    vec3 msd = texture(uAtlas, vUV).rgb;
    float sd = median(msd.r, msd.g, msd.b) - 0.5;
    vec2 unit_range = vec2(uRange) / vec2(textureSize(uAtlas, 0));
    vec2 screen_tex_size = vec2(1.0) / fwidth(vUV);
    float px_range = max(0.5 * dot(unit_range, screen_tex_size), 1.0);
    float alpha = clamp(sd * px_range + 0.5, 0.0, 1.0);
    fragColor = vec4(uColor.rgb, uColor.a * alpha);
}
)";
}  // namespace

void TextLabelNode::operator()(double /*time_s*/) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});

    std::string text = endpoints.text.get();
    if (text.empty()) text = "label";

    std::vector<float> quads;  // x, y, u, v per vertex
    glyph::layout(text, quads);

    const float     ox = endpoints.pos_x.get(), oy = endpoints.pos_y.get(), oz = endpoints.pos_z.get();
    const float     sc = endpoints.scale.get();
    auto            data = std::make_shared<TriMeshData>();
    data->vertices.reserve(quads.size() / 4);
    for (std::size_t i = 0; i + 3 < quads.size(); i += 4) {
        TriVertex v;
        v.position = Eigen::Vector3f(ox + quads[i] * sc, oy + quads[i + 1] * sc, oz);
        v.normal   = Eigen::Vector3f(quads[i + 2], quads[i + 3], 0.f);  // UV
        v.color    = Eigen::Vector4f(1.f, 1.f, 1.f, 1.f);
        data->vertices.push_back(v);
    }

    Mesh m;
    m.geometry = std::move(data);  // no indices → draw_arrays(TRIANGLES)
    m.mode     = Primitive::Triangles;
    m.dynamic  = true;
    endpoints.mesh.value = std::move(m);

    const glyph::AtlasImage& atlas = glyph::atlas();
    Surface                  s;
    s.shader      = shader_;
    s.blend       = true;
    s.depth_write = false;  // text overlay; tests depth but doesn't occlude
    s.cull_back   = false;
    s.uniforms.push_back({"uRange", atlas.distance_range});
    s.uniforms.push_back({"uColor", Eigen::Vector4f(1.f, 1.f, 1.f, 1.f)});
    s.images.push_back({"uAtlas", atlas.pixels, atlas.pixels, atlas.width, atlas.height, atlas.channels});
    endpoints.surface.value = std::move(s);
}
