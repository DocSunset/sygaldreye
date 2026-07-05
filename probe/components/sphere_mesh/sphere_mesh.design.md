# sphere_mesh

Owning package: `render`

GPU mesh for a sphere: owns a VAO/VBO/EBO and issues a single `glDrawElements` call.

## Ports

- **Inputs**: `SphereGeometry` provided at creation time (CPU data uploaded to GPU once).
- **Outputs**: draw call issued via `draw()`.
- **Sources**: OpenGL ES 3 context must be current when `create()` or `draw()` is called.
- **Destinations**: attribute layout (location 0 = position, location 1 = normal, stride 24) must match the consuming shader (e.g. `lit_shader`).
- **Temporal couplings**: `create()` must be called before `draw()`; GL context must remain valid for the lifetime of the object.
- **Intended seams**: none.

## Requirements

- Upload interleaved `{position(vec3), normal(vec3)}` = 24 bytes per vertex.
- Render with `glDrawElements(GL_TRIANGLES, …, GL_UNSIGNED_SHORT, nullptr)`.
- Release all GL objects on destruction.
- Move-constructible and move-assignable; no copy.

## Allowed dependencies

- `sphere_geometry`
- `gl_program` (indirect via OpenGL ES 3 headers)
