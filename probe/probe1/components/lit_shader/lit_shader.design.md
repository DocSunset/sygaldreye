# `lit_shader` Design

## Owning package

`render`

## Allowed dependencies

- `gl_program`
- `light`
- `material`

## Ports

### Inputs

- `init()`: compile GLSL shaders and cache all uniform locations. Must be called once after an OpenGL ES context is active.
- `set_mvp(Matrix4f)`: model-view-projection matrix for vertex transformation.
- `set_model(Matrix4f)`: model matrix used to compute world-space positions and the normal matrix.
- `set_view_pos(Vector3f)`: camera/eye position in world space for specular calculation.
- `set_lights(span<const Light>)`: up to 8 directional lights.
- `set_material(Material)`: ambient, diffuse, specular, and shininess coefficients.

### Outputs

- `use()`: binds the shader program for subsequent draw calls.

### Sources

- Receives compiled shader from `GlProgram::build`.

### Destinations

- Produces GLSL draw output consumed by the GPU framebuffer.

### Temporal couplings

- `init()` must precede all `use()` / `set_*()` calls.
- `use()` must be called before `set_*()` calls take effect for a draw.

### Intended seams

None.

## Requirements

- Implement Blinn-Phong shading with up to 8 directional lights.
- Accept `Light` and `Material` structs via their public headers.
- Forward-declare `Light` and `Material` in the public header to avoid transitive includes.
- Cache all uniform locations at `init()` time for low-overhead per-frame updates.
