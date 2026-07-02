// Copyright 2025 Travis West
#include "text_label.hpp"

#include <memory>
#include <string>
#include <vector>

#include <Eigen/Core>

#include "glyph_layout.hpp"
#include "tri_mesh.hpp"

#include "common_shaders.hpp"

void TextLabelNode::operator()(double /*time_s*/) {
    if (!shader_)
        shader_ = std::make_shared<ShaderData>(
            ShaderData{common_shaders::kMsdfTextVert, common_shaders::kMsdfTextFrag});

    std::string text = endpoints.text.get();
    if (text.empty()) text = "label";

    std::vector<float> quads;  // x, y, u, v per vertex
    glyph::layout(text, quads);

    const float     ox = endpoints.pos_x.get(), oy = endpoints.pos_y.get(), oz = endpoints.pos_z.get();
    const float     sc = endpoints.scale.get();
    if (!data_) data_ = std::make_shared<TriMeshData>();
    auto& data = data_;
    data->vertices.clear();
    data->vertices.reserve(quads.size() / 4);
    for (std::size_t i = 0; i + 3 < quads.size(); i += 4) {
        TriVertex v;
        v.position = Eigen::Vector3f(ox + quads[i] * sc, oy + quads[i + 1] * sc, oz);
        v.normal   = Eigen::Vector3f(quads[i + 2], quads[i + 3], 0.f);  // UV
        v.color    = Eigen::Vector4f(1.f, 1.f, 1.f, 1.f);
        data->vertices.push_back(v);
    }

    data->touch();

    Mesh m;
    m.geometry = data_;  // no indices → draw_arrays(TRIANGLES)
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
