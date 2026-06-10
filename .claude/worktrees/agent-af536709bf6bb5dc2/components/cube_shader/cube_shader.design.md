# cube_shader

Owning package: `render`

Owns the GLSL source strings and the compiled `GlProgram` for the cube renderer.

## Ports

**Inputs:**
- `init()` — called once after EGL/GL context is active; compiles and links the shader program, caches the MVP uniform location.
- `use()` — binds the program for subsequent draw calls.
- `set_mvp(mvp)` — uploads the MVP matrix to the cached uniform location.

**Outputs:** none

**Sources:**
- Active OpenGL ES context (provided by `renderer` before `init`/`use`/`set_mvp` are called)

**Destinations:** none

**Temporal couplings:**
- `init()` must be called before `use()` or `set_mvp()`
- Must be called within an active GL context

**Intended seams:** none

## Requirements

- Compiles a vertex shader that transforms positions by a `uMVP` matrix and passes per-vertex color through.
- Compiles a fragment shader that outputs the interpolated color.
- Caches the `uMVP` uniform location to avoid per-draw lookups.

## Allowed dependencies

- `gl_program` component
- Eigen (math types only)
- Android log (`log`)
