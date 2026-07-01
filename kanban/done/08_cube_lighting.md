# cube lighting rework

Retrofit the existing `cube_geometry` and `cube_shader` components to support Blinn-Phong lighting via `lit_shader`. Currently cube vertices carry only position and color; normals are absent and the shader has no concept of light.

## Approach

- **`cube_geometry`**: add a normal (vec3) per vertex. Each face of the cube has a constant outward normal; store it per-vertex alongside position. Drop the per-vertex color attribute — material color comes from `Material` uniforms now.
- **`cube_shader`**: retire or gut. Replace with `lit_shader` at the call site (`cube_mesh`). `CubeMesh::draw()` gains a `Material` parameter and delegates to `LitShader`.
- **VAO layout**: interleaved position (location=0) + normal (location=1), matching `lit_shader` exactly.
- **`CubeInstance`**: already carries a `Material` field (from item 03).
- App layer passes scene lights and per-cube material to `lit_shader` each draw call.

## Acceptance

- Rotating world cube shows a visible Blinn-Phong highlight under the scene's directional light
- Hand cubes are also lit
- No per-vertex color remains in cube geometry
- No GL errors
- `cube_geometry.design.md` and `cube_shader` docs updated to reflect the change (or `cube_shader` removed and the design doc updated accordingly)

## Dependencies

- `lit_shader` (item 04)
- `material` (item 03)
- `light` (item 02)
