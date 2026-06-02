# cube_mesh

Owning package: `render`

A unit cube mesh with per-face colors, rendered via OpenGL ES 3.2.

## Ports

**Inputs:**
- `init()` — called once after EGL/GL context is active; builds GPU buffers and compiles shaders.
- `draw(mvp)` — called each frame with a combined model-view-projection matrix.

**Outputs:** none (draws directly to the currently bound framebuffer)

**Sources:**
- Active OpenGL ES context (provided by `renderer` before `init`/`draw` are called)

**Destinations:** none

**Temporal couplings:**
- `init()` must be called before `draw()`
- Must be called within an active GL context (EGL surface current)

**Intended seams:** none

## Requirements

- Renders a unit cube (±1 on each axis) with per-face solid colors: +X red, -X cyan, +Y green, -Y magenta, +Z blue, -Z yellow.
- Accepts an MVP matrix; applies it in the vertex shader.
- No external file I/O; all geometry and shaders are compiled in.

## Allowed dependencies

- OpenGL ES 3.2 (`GLESv3`)
- `gl_program` component
- Eigen (math types only, no GL dependency)
- Android log (`log`)
