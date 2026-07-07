# cylinder_mesh component

A GPU mesh for a unit cylinder (radius 1, length 1 along Z, centred at origin). The rubber band is rendered by transforming this mesh to span two world-space endpoints.

## Approach

- `cylinder_mesh.hpp` declares `CylinderMesh` with `create(int slices)` and `draw()`.
- `cylinder_mesh.cpp` generates vertices procedurally (two end caps + side quads), uploads to VAO/VBO/EBO.
  - Each vertex carries a normal: radial (pointing outward from axis) for side vertices, axial (±Z) for cap vertices.
  - Interleaved layout: position (vec3) then normal (vec3), matching `lit_shader` attribute locations (position=0, normal=1).
- A helper free function `cylinder_transform(Eigen::Vector3f a, Eigen::Vector3f b, float radius) -> Eigen::Matrix4f` computes the model matrix that maps the unit cylinder onto the segment from `a` to `b` at the given radius. Lives in `cylinder_mesh.hpp`.

## Acceptance

- `cylinder_transform` places the cylinder visually between two known points (verify on-device)
- Normals are visually correct under `lit_shader`: sides respond to light from the side, caps respond to axial light
- No GL errors from `draw()`
- `cylinder_mesh.design.md` present; each `.cpp` under 100 lines of substantive code

## Dependencies

- `lit_shader` (item 04)
