# cube_geometry

Owning package: `render`

Owns the VAO, VBO, and EBO for the unit cube, and the static vertex and index data.

## Ports

**Inputs:**
- `init()` — called once after EGL/GL context is active; uploads geometry data and configures vertex attribute pointers.
- `bind()` — binds the VAO for subsequent draw calls.
- `unbind()` — unbinds the VAO.
- `draw_elements()` — issues the indexed draw call.

**Outputs:** none (draws directly to the currently bound framebuffer when `draw_elements` is called)

**Sources:**
- Active OpenGL ES context (provided by `renderer` before any method is called)

**Destinations:** none

**Temporal couplings:**
- `init()` must be called before `bind()` or `draw_elements()`
- `bind()` must be called before `draw_elements()`
- Must be called within an active GL context

**Intended seams:** none

## Requirements

- Stores 24 vertices (4 per face, 6 faces) with interleaved position (vec3) and color (vec3).
- Stores 36 indices (2 triangles per face, 6 faces) as unsigned shorts.
- Per-face colors: +X red, -X cyan, +Y green, -Y magenta, +Z blue, -Z yellow.

## Allowed dependencies

- OpenGL ES 3.2 (`GLESv3`)
- Android log (`log`)
