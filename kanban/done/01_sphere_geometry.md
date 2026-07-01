# sphere_geometry component

Generate sphere vertex and index data procedurally (longitude/latitude tessellation). Parallel to `cube_geometry`: produces a plain struct with positions, normals, and triangle indices — no GPU objects.

## Approach

- `sphere_geometry.hpp` declares `SphereGeometry` with `vertices` (position + normal per vertex) and `indices` (triangle list), and a `make_sphere(int lon_slices, int lat_slices) -> SphereGeometry` factory.
- `sphere_geometry.cpp` implements the lon/lat tessellation math.
- `sphere_geometry.test.cpp`: verify vertex count, index count, that every index is in range, and that all vertex normals are unit length.

## Acceptance

- `make_sphere(32, 16)` produces the expected vertex and index counts
- All normals have magnitude within 1e-5 of 1.0
- Host-buildable and host-testable (no GL, no XR)
- `sphere_geometry.design.md` present; each `.cpp` under 100 lines of substantive code
