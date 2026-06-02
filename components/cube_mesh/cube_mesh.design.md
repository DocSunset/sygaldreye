# cube_mesh

Owning package: `render`

Composes `cube_shader` and `cube_geometry` to present a single unit cube rendering interface.

## Ports

**Inputs:**
- `init()` — called once after EGL/GL context is active; delegates to shader and geometry init.
- `begin_batch()` — activates the shader program and binds the VAO; call once before one or more `draw()` calls.
- `end_batch()` — unbinds the VAO; call after all `draw()` calls in a batch.
- `draw(mvp)` — sets the MVP uniform and issues the draw call; must be called between `begin_batch` and `end_batch`.

**Outputs:** none (draws directly to the currently bound framebuffer)

**Sources:**
- Active OpenGL ES context (provided by `renderer` before `init`/`begin_batch`/`draw` are called)

**Destinations:** none

**Temporal couplings:**
- `init()` must be called before `begin_batch()` or `draw()`
- `begin_batch()` must be called before `draw()`; `end_batch()` must be called after
- Must be called within an active GL context

**Intended seams:** none

## Requirements

- Renders a unit cube (±1 on each axis) with per-face solid colors: +X red, -X cyan, +Y green, -Y magenta, +Z blue, -Z yellow.
- Accepts an MVP matrix per draw call.
- Public API unchanged from the monolithic version.

## Allowed dependencies

- `cube_shader` component
- `cube_geometry` component
- Eigen (math types only)
