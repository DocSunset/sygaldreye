#!/usr/bin/env python3
"""Generate dejavu_sans_metrics.cpp and dejavu_sans_png.cpp from atlas JSON + PNG.

Usage: gen_font_cpp.py <atlas.json> <atlas.png> <output_dir>
"""

import json
import sys


def fstr(val: float) -> str:
    """Format a float as a C float literal (always has decimal point)."""
    if val == float(int(val)):
        return f"{int(val)}.0F"
    return f"{val}F"


def gen_metrics(atlas_json: str, out_dir: str) -> None:
    with open(atlas_json) as fh:
        d = json.load(fh)

    atlas   = d["atlas"]
    metrics = d["metrics"]
    glyphs  = d["glyphs"]
    glyph_map = {g["unicode"]: g for g in glyphs}

    lines = [
        "// GENERATED FILE — do not edit by hand.",
        "// Re-generate via: sh/gen_font_atlas.sh",
        '#include "dejavu_sans_atlas.hpp"',
        "",
        "namespace dejavu_sans {",
        "",
        "const AtlasInfo ATLAS_INFO = {",
        f"    .width          = {atlas['width']},",
        f"    .height         = {atlas['height']},",
        f"    .distance_range = {fstr(atlas['distanceRange'])},",
        f"    .line_height    = {fstr(metrics['lineHeight'])},",
        f"    .ascender       = {fstr(metrics['ascender'])},",
        f"    .descender      = {fstr(metrics['descender'])},",
        "};",
        "",
        "const GlyphInfo GLYPHS[NUM_GLYPHS] = {",
    ]

    for codepoint in range(32, 127):
        glyph   = glyph_map.get(codepoint, {})
        advance = glyph.get("advance", 0.0)
        plane   = glyph.get("planeBounds", None)
        ab      = glyph.get("atlasBounds", None)
        ch      = chr(codepoint)
        label   = repr(ch) if codepoint not in (39, 92) else f"'\\{ch}'"
        if plane and ab:
            lines.append(
                f"    /* {label} */ {{ {fstr(advance)}, "
                f"{fstr(plane['left'])}, {fstr(plane['bottom'])}, "
                f"{fstr(plane['right'])}, {fstr(plane['top'])}, "
                f"{fstr(ab['left'])}, {fstr(ab['bottom'])}, "
                f"{fstr(ab['right'])}, {fstr(ab['top'])} }},"
            )
        else:
            lines.append(
                f"    /* {label} */ "
                f"{{ {fstr(advance)}, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F }},"
            )

    lines += ["};", "", "} // namespace dejavu_sans", ""]

    out_path = f"{out_dir}/dejavu_sans_metrics.cpp"
    with open(out_path, "w") as fh:
        fh.write("\n".join(lines))
    print(f"Written: {out_path}")


def gen_png_cpp(atlas_png: str, out_dir: str) -> None:
    with open(atlas_png, "rb") as fh:
        png_data = fh.read()

    lines = [
        "// GENERATED FILE — do not edit by hand.",
        "// Re-generate via: sh/gen_font_atlas.sh",
        '#include "dejavu_sans_atlas.hpp"',
        "",
        "namespace dejavu_sans {",
        "",
        f"const int PNG_SIZE = {len(png_data)};",
        "",
        "// clang-format off",
        f"const uint8_t PNG_DATA[{len(png_data)}] = {{",
    ]

    per_line = 16
    for offset in range(0, len(png_data), per_line):
        chunk = png_data[offset:offset + per_line]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")

    lines += ["};", "// clang-format on", "", "} // namespace dejavu_sans", ""]

    out_path = f"{out_dir}/dejavu_sans_png.cpp"
    with open(out_path, "w") as fh:
        fh.write("\n".join(lines))
    print(f"Written: {out_path}")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <atlas.json> <atlas.png> <output_dir>")
        sys.exit(1)
    gen_metrics(sys.argv[1], sys.argv[3])
    gen_png_cpp(sys.argv[2], sys.argv[3])
