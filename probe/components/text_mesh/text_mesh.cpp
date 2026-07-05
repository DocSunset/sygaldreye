// Copyright 2026 Travis West
// CPU glyph layer (render-as-nodes): no GL. Decodes the MSDF atlas and lays
// out glyph quads; shared by text_label, card_labels_mesh, palette_mesh. The
// old GL TextMesh class was deleted with the editor monolith (DrawFn leg).
#include "glyph_layout.hpp"

#include <string_view>
#include <vector>

#include "dejavu_sans_atlas.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif
#include "stb_image.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace glyph {

const AtlasImage& atlas() {
    static AtlasImage img = [] {
        AtlasImage a;
        int        ch = 0;
        a.pixels = stbi_load_from_memory(dejavu_sans::PNG_DATA, dejavu_sans::PNG_SIZE, &a.width,
                                         &a.height, &ch, 3);
        a.channels       = 3;
        a.distance_range = dejavu_sans::ATLAS_INFO.distance_range;
        return a;
    }();
    return img;
}

void layout(std::string_view text, std::vector<float>& out) {
    const auto& info  = dejavu_sans::ATLAS_INFO;
    const float inv_w = 1.0F / static_cast<float>(info.width);
    const float inv_h = 1.0F / static_cast<float>(info.height);
    float       cursor = 0.0F;
    for (unsigned char uch : text) {
        int cp = static_cast<int>(uch);
        if (cp < dejavu_sans::FIRST_CODEPOINT || cp > dejavu_sans::LAST_CODEPOINT)
            cp = static_cast<int>('?');
        const auto& g = dejavu_sans::GLYPHS[cp - dejavu_sans::FIRST_CODEPOINT];
        if (g.plane_right > g.plane_left) {
            float xl = cursor + g.plane_left, xr = cursor + g.plane_right;
            float yb = g.plane_bottom, yt = g.plane_top;
            float ul = g.atlas_left * inv_w, ur = g.atlas_right * inv_w;
            float vb = 1.0F - g.atlas_bottom * inv_h, vt = 1.0F - g.atlas_top * inv_h;
            const float quad[24] = {xl, yb, ul, vb, xr, yb, ur, vb, xr, yt, ur, vt,
                                    xl, yb, ul, vb, xr, yt, ur, vt, xl, yt, ul, vt};
            out.insert(out.end(), quad, quad + 24);
        }
        cursor += g.advance;
    }
}

}  // namespace glyph
