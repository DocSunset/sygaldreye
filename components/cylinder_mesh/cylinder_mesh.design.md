# cylinder_mesh

Procedurally generates a unit cylinder as CPU geometry (radius=1, Z in
[-0.5,+0.5], centred at origin) — a shareable `TriMeshData`, the
render-as-nodes mesh payload. GL lives only in render_region; this is pure
geometry (ABI v8).

## Ports

### Inputs
- `make_cylinder(int slices)` — number of longitudinal slices; higher =
  smoother cylinder.

### Outputs
- A `MeshPtr` (`shared_ptr<const TriMeshData>`): position+normal+color
  vertices and triangle indices. A consumer wraps it in a `Mesh` for a
  `draw` node.

### Sources / Destinations
- None; no GL, no context coupling.

### Temporal Couplings
- None.

### Intended Seams
- None; geometry is fixed procedural.

## Requirements

- Unit cylinder: radius=1, Z in [-0.5, +0.5], centred at origin.
- `TriVertex` layout `{position, normal, color}`; index type `uint32_t`.
- Side normals are radial; cap normals are axial.
- `cylinder_transform(a, b, radius)` returns a 4×4 model matrix that maps the
  unit cylinder onto the segment from `a` to `b` at the given radius.

## Allowed Dependencies

- `tri_mesh` (TriMeshData)
- `Eigen` (geometry math)
- C++ standard library (`<cmath>`, `<numbers>`, `<memory>`)

## Owning Package

`render`
