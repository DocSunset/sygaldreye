# sphere_geometry

Procedurally generates sphere vertex and index data using longitude/latitude tessellation. Pure CPU math — no GPU, no XR, no Android headers.

## Ports

**Inputs:** `lon_slices`, `lat_slices` — tessellation resolution parameters passed to `make_sphere`.

**Outputs:** `SphereGeometry` — struct containing `vertices` (position + normal) and `indices` (triangle list).

No sources, destinations, temporal couplings, or intended seams.

## Requirements

- `make_sphere(lon, lat)` produces `(lon+1)*(lat+1)` vertices and `lon*lat*6` indices.
- All vertex normals are unit length.
- All indices are in range `[0, vertex_count)`.
- Indices are `uint16_t`; caller must ensure vertex count fits.

## Allowed dependencies

- Eigen (math only)
- C++ standard library

## Owning package

`scene`
