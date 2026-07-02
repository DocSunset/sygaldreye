// Copyright 2026 Travis West
#include "palette_mesh.hpp"

#include <cstdio>
#include <memory>
#include <vector>

#include "common_shaders.hpp"
#include "glyph_layout.hpp"
#include "palette.hpp"
#include "tri_mesh.hpp"

namespace {

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
        panel_shader_ = std::make_shared<ShaderData>(ShaderData{
            common_shaders::kUnlitVertexColorVert, common_shaders::kUnlitVertexColorFrag});
    if (!text_shader_)
        text_shader_ = std::make_shared<ShaderData>(
            ShaderData{common_shaders::kMsdfTextVert, common_shaders::kMsdfTextFrag});

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
