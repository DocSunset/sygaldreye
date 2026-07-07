// Copyright 2026 Travis West
#pragma once
#include <string_view>
#include <vector>

// CPU glyph layer (no GL): the MSDF atlas pixels + glyph quad layout, shared by
// the text node (which emits a Mesh + atlas-image Surface for render_region)
// and anything else that draws text. The atlas is decoded once on first use.
namespace glyph {

struct AtlasImage {
    const unsigned char* pixels = nullptr;
    int                  width = 0, height = 0, channels = 0;
    float                distance_range = 0.f;
};

// Decoded once; pixels are valid for the program's lifetime.
const AtlasImage& atlas();

// Append the quads for `text` (baseline at origin, +X advance, em units). Each
// glyph is 6 vertices; each vertex is 4 floats: x, y, u, v.
void layout(std::string_view text, std::vector<float>& out);

}  // namespace glyph
