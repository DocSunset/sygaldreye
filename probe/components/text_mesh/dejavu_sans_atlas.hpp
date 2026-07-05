// GENERATED FILE — do not edit by hand.
#pragma once
#include <cstdint>

namespace dejavu_sans {

struct AtlasInfo {
    int   width;
    int   height;
    float distance_range;
    float line_height;   // em units
    float ascender;      // em units
    float descender;     // em units
};

struct GlyphInfo {
    float advance;         // em units
    float plane_left;      // em units (quad offset from cursor)
    float plane_bottom;
    float plane_right;
    float plane_top;
    float atlas_left;      // pixels
    float atlas_bottom;
    float atlas_right;
    float atlas_top;
};

constexpr int FIRST_CODEPOINT = 32;
constexpr int LAST_CODEPOINT  = 126;
constexpr int NUM_GLYPHS = LAST_CODEPOINT - FIRST_CODEPOINT + 1;

extern const AtlasInfo  ATLAS_INFO;
extern const GlyphInfo  GLYPHS[NUM_GLYPHS];
extern const uint8_t    PNG_DATA[];
extern const int        PNG_SIZE;

} // namespace dejavu_sans
