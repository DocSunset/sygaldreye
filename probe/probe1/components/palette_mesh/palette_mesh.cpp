// Copyright 2026 Travis West
#include "palette_mesh.hpp"

#include <cstdio>
#include <memory>
#include <vector>

#include "glyph_layout.hpp"
#include "palette.hpp"
#include "tri_mesh.hpp"

namespace {
// Unlit vertex-color shader for the backdrop panel.
constexpr const char* kPanelVert = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec4 aColor;
uniform mat4 uMVP;
out vec4 vColor;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); vColor = aColor; }
)";
constexpr const char* kPanelFrag = R"(#version 300 es
precision mediump float;
in vec4 vColor;
out vec4 fragColor;
void main() { fragColor = vColor; }
)";
constexpr const char* kTextVert = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uMVP;
out vec2 vUV;
void main() { gl_Position = uMVP * vec4(aPos, 1.0); vUV = aNormal.xy; }
)";
constexpr const char* kTextFrag = R"(#version 300 es
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

void quad(
    TriMeshData& m, float cx, float cy, float cz, float hw, float hh, const Eigen::Vector4f& col) {
    const Eigen::Vector3f n{0, 0, 1};
    const float dx[4] = {-hw, hw, hw, -hw};
    const float dy[4] = {-hh, -hh, hh, hh};
    auto base = std::uint32_t(m.vertices.size());
    for (int i = 0; i < 4; ++i) m.vertices.push_back({{cx + dx[i], cy + dy[i], cz}, n, col});
    for (std::uint32_t i : {0u, 1u, 2u, 0u, 2u, 3u}) m.indices.push_back(base + i);
}
}  // namespace

void PaletteMeshNode::operator()(double) {
    if (!panel_shader_)
        panel_shader_ = std::make_shared<ShaderData>(ShaderData{kPanelVert, kPanelFrag});
    if (!text_shader_)
        text_shader_ = std::make_shared<ShaderData>(ShaderData{kTextVert, kTextFrag});

    const Eigen::Vector3f pp = PaletteNode::panel_pos();
    const float pw = PaletteNode::panel_w(), ph = PaletteNode::panel_h();
    const int rows = PaletteNode::kRows;
    const auto* types = ctx_.types;
    int page = int(endpoints.page.get() + 0.5f);
    int npages = types && !types->empty() ? (int(types->size()) + rows - 1) / rows : 1;
    if (npages < 1) npages = 1;
    if (page < 0 || page >= npages) page = 0;

    // Backdrop panel.
    auto panel = std::make_shared<TriMeshData>();
    quad(*panel, pp.x(), pp.y(), pp.z(), pw * 0.5f, ph * 0.5f, {0.05f, 0.08f, 0.12f, 0.90f});
    Mesh pm;
    pm.geometry = std::move(panel);
    pm.dynamic = true;
    endpoints.panel.value = std::move(pm);
    Surface ps;
    ps.shader = panel_shader_;
    ps.blend = true;
    endpoints.panel_surface.value = std::move(ps);

    // Type-row labels (header row flips pages, then this page's slice).
    auto text = std::make_shared<TriMeshData>();
    std::vector<float> quads;
    char hdr[40];
    std::snprintf(hdr, sizeof(hdr), "more... %d/%d", page + 1, npages);
    for (int r = 0; r <= rows; ++r) {
        int idx = page * rows + r - 1;
        if (r > 0 && (!types || idx >= int(types->size()))) break;
        const std::string& label = (r == 0) ? std::string(hdr) : (*types)[std::size_t(idx)];
        float v = (float(r) + 0.5f) / float(rows + 1);
        float y = pp.y() + ph * (0.5f - v);
        float ox = pp.x() - pw * 0.45f;
        float oz = pp.z() + 0.002f;
        const float sc = 0.02f * 0.5f;  // 0.02 m * text_scale parity
        quads.clear();
        glyph::layout(label, quads);
        for (std::size_t i = 0; i + 3 < quads.size(); i += 4) {
            TriVertex tv;
            tv.position = {ox + quads[i] * sc, y + quads[i + 1] * sc, oz};
            tv.normal = {quads[i + 2], quads[i + 3], 0.f};
            tv.color = {1.f, 1.f, 1.f, 1.f};
            text->vertices.push_back(tv);
        }
    }
    Mesh lm;
    lm.geometry = std::move(text);
    lm.dynamic = true;
    endpoints.labels.value = std::move(lm);

    const glyph::AtlasImage& atlas = glyph::atlas();
    Surface ls;
    ls.shader = text_shader_;
    ls.blend = true;
    ls.depth_write = false;
    ls.cull_back = false;
    ls.uniforms.push_back({"uRange", atlas.distance_range});
    ls.uniforms.push_back({"uColor", Eigen::Vector4f(1.f, 1.f, 1.f, 1.f)});
    ls.images.push_back(
        {"uAtlas", atlas.pixels, atlas.pixels, atlas.width, atlas.height, atlas.channels});
    endpoints.labels_surface.value = std::move(ls);
}
