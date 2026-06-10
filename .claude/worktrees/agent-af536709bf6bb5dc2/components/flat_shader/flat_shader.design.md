# flat_shader

GL shader program that renders geometry with per-vertex color using flat (face-constant) interpolation. Defines the low-poly visual style: each triangle displays a single solid color.

## Ports

### Inputs
- `set_mvp(Matrix4f)`: model-view-projection matrix for the next draw call

### Outputs
- Fragment color output to the framebuffer

### Sources
- `create()`: must be called after an OpenGL ES context is current
- `use()`: must be called before draw calls and `set_mvp`

### Temporal couplings
- `create()` before `use()` or `set_mvp()`
- OpenGL ES context must be current for all methods

## Requirements

- Per-vertex RGBA color at attribute location 1 (no uniform color)
- GLSL `flat` interpolation: provoking vertex color fills the whole triangle
- MVP transform applied to position (attribute location 0, vec3)
- No lighting computation

## Allowed dependencies

- `gl_program`

## Owning package

`render`
