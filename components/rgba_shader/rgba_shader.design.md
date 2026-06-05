# rgba_shader

## Owning package
`render`

## Allowed dependencies
- `gl_program`

## Ports

### Inputs
- `create()`: compiles and links GLSL shaders; must be called once after an EGL context is current.
- `use()`: binds the shader program for subsequent draw calls.
- `set_mvp(Matrix4f)`: uploads the model-view-projection matrix uniform.
- `set_color(Vector4f)`: uploads the RGBA color uniform.

### Outputs
- Compiled GL program object (owned internally).

### Temporal couplings
- `create()` must precede `use()`, `set_mvp()`, and `set_color()`.
- An EGL context must be current during `create()` and all draw-time calls.

## Requirements
- Render geometry with a single uniform RGBA color (no per-vertex color).
- Support translucent geometry (alpha < 1); blending is the caller's responsibility.
- Move-constructible and move-assignable; not copyable.
