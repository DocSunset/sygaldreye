// Copyright 2026 Travis West
#include "card_labels_mesh.hpp"

#include <memory>
#include <vector>

#include "common_shaders.hpp"
#include "glyph_layout.hpp"
#include "tri_mesh.hpp"

void CardLabelsMeshNode::operator()(double) {
    if (!shader_)
        shader_ = std::make_shared<ShaderData>(
            ShaderData{common_shaders::kMsdfTextVert, common_shaders::kMsdfTextFrag});
    auto data = std::make_shared<TriMeshData>();

    if (ctx_.graph) {
        const editor_layout::Layout& l = editor_layout::cached_layout(ctx_);
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
