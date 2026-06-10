# text_mesh design

## Owning package

`render`

## Allowed dependencies

- `gl_program` (shader compilation/linking)
- `stb_image.h` (vendored single-header PNG decoder — compile-time only via `STB_IMAGE_IMPLEMENTATION`)
- Android NDK: `GLES3/gl3.h`, `android/log.h`
- Eigen: `Eigen/Core`

## Ports

### Inputs

- `init()` — called once after an OpenGL ES context is current. Compiles shaders, uploads atlas texture, creates VAO/VBO.
- `draw(text, mvp)` — called each frame inside the render loop. Lays out glyph quads and draws them.

### Outputs

- Renders MSDF-textured quads to the current framebuffer.

### Temporal couplings

- `init()` must be called before `draw()`.
- An OpenGL ES context must be current on both calls.

## Requirements

- Text renders at sharp, resolution-independent quality at any viewing distance (MSDF anti-aliasing in fragment shader).
- Text is **emissive (unlit)**. The fragment shader outputs text color directly with no light attenuation. Rationale: text legibility must be consistent regardless of orientation — a label that's dark because it's facing away from a light is a bug, not a feature.
- No runtime font rasterization. The atlas PNG and glyph metrics are pre-generated offline and committed as source.
- Callers need no GL knowledge; only a world-space MVP matrix is required.
- The baseline starts at the origin. Glyphs extend in the +X direction. Scale text via the MVP transform (1 em = 1 unit in model space).

## Atlas generation (offline, host tool)

```sh
sh/gen_font_atlas.sh
```

This script:
1. Runs `msdf-atlas-gen` (available in the nix devShell) on `DejaVu Sans Regular`.
2. Produces `assets/fonts/dejavu_sans.png` (256×256 MSDF atlas, RGB).
3. Produces `assets/fonts/dejavu_sans.json` (glyph metrics).
4. Regenerates `components/text_mesh/dejavu_sans_metrics.cpp` and `components/text_mesh/dejavu_sans_png.cpp`.

The generated `.cpp` files are committed to source control so no tool is required at build time.

## MSDF fragment shader

The fragment shader samples all three RGB channels, computes the median (which is the MSDF signal), then uses `smoothstep` with a screen-space derivative to produce anti-aliased alpha:

```glsl
float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}
float sd = median(msd.r, msd.g, msd.b);
float screen_px = uRange * length(vec2(dFdx(sd), dFdy(sd))) * 0.7071;
float alpha = smoothstep(0.5 - screen_px, 0.5 + screen_px, sd);
```

`uRange` is the atlas distance range in pixels (2 for this atlas).

## Lighting policy

Emissive. See requirements above.
