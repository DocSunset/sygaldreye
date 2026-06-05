# cylinder_mesh

Procedurally generates a unit cylinder GPU mesh (radius=1, length=1 along Z, centred at origin) and uploads it to OpenGL ES buffers for rendering.

## Ports

### Inputs
- `create(int slices)` — number of longitudinal slices; higher = smoother cylinder.

### Outputs
- `draw()` — issues a `glDrawElements` call; must be called within an active OpenGL ES context with `lit_shader` bound.

### Sources
- OpenGL ES context must be current when `create()` or `draw()` is called.

### Destinations
- Vertex attribute layout matches `lit_shader`: position at location 0, normal at location 1, interleaved 24-byte stride.

### Temporal Couplings
- `create()` must be called before `draw()`.
- The OpenGL ES context used for `create()` must still be current at `draw()` time.

### Intended Seams
- None; geometry is fixed procedural.

## Requirements

- Unit cylinder: radius=1, Z in [-0.5, +0.5], centred at origin.
- Interleaved vertex layout: `{position(vec3), normal(vec3)}`, 24 bytes per vertex.
- Side normals are radial; cap normals are axial.
- Index type: `uint16_t`.
- Move-only ownership of GPU resources (VAO, VBO, EBO).
- `cylinder_transform(a, b, radius)` returns a 4×4 model matrix that maps the unit cylinder onto the segment from `a` to `b` at the given radius.

## Allowed Dependencies

- `GLES3/gl3.h` (OpenGL ES 3.2)
- `Eigen` (geometry math)
- `android/log.h`
- C++ standard library (`<cmath>`, `<numbers>`, `<vector>`)

## Owning Package

`render`
