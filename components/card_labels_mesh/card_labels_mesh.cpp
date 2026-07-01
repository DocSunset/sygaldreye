// Copyright 2026 Travis West
#include "card_labels_mesh.hpp"

#include <memory>
#include <vector>

#include "glyph_layout.hpp"
#include "tri_mesh.hpp"

namespace {
// MSDF text shader (the canonical glyph shader; UV carried in aNormal.xy).
constexpr const char* kVert = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uMVP;
out vec2 vUV;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); vUV = aNormal.xy; }
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

void CardLabelsMeshNode::operator()(double) {
    if (!shader_) shader_ = std::make_shared<ShaderData>(ShaderData{kVert, kFrag});
    auto data = std::make_shared<TriMeshData>();

    if (ctx_.graph) {
        editor_layout::Layout l = editor_layout::build_layout(
            *ctx_.graph, ctx_.overrides ? *ctx_.overrides : editor_layout::PosOverrides{});
        const float sc = endpoints.scale.get() * 0.018f;  // 0.018 m * text_scale parity
        std::vector<float> quads;                         // x, y, u, v per vertex
        for (const auto& card : l.cards) {
            quads.clear();
            glyph::layout(card.node_id, quads);
            const float ox = card.position.x() - card.width * 0.45f;
            const float oy = card.position.y() + card.height * 0.4f;
            const float oz = card.position.z() + 0.002f;
            for (std::size_t i = 0; i + 3 < quads.size(); i += 4) {
                TriVertex v;
                v.position = {ox + quads[i] * sc, oy + quads[i + 1] * sc, oz};
                v.normal = {quads[i + 2], quads[i + 3], 0.f};  // UV
                v.color = {1.f, 1.f, 1.f, 1.f};
                data->vertices.push_back(v);
            }
        }
    }

    Mesh m;
    m.geometry = std::move(data);  // no indices → draw_arrays(TRIANGLES)
    m.mode = Primitive::Triangles;
    m.dynamic = true;
    endpoints.mesh.value = std::move(m);

    const glyph::AtlasImage& atlas = glyph::atlas();
    Surface s;
    s.shader = shader_;
    s.blend = true;
    s.depth_write = false;
    s.cull_back = false;
    s.uniforms.push_back({"uRange", atlas.distance_range});
    s.uniforms.push_back({"uColor", Eigen::Vector4f(1.f, 1.f, 1.f, 1.f)});
    s.images.push_back(
        {"uAtlas", atlas.pixels, atlas.pixels, atlas.width, atlas.height, atlas.channels});
    endpoints.surface.value = std::move(s);
}
