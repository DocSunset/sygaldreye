# terrain_generator

Procedurally generates a flat-shaded colored triangle mesh for a terrain patch using fBm noise for height.

## Ports

**Inputs:**
- `TerrainParams` struct passed by value to `generate_terrain`

**Outputs:**
- `TriMeshData` returned by value

**Sources:** none

**Destinations:** caller uploads `TriMeshData` to GPU via `TriMesh`

**Temporal couplings:** none

**Intended seams:** `TerrainParams::band_colors` and noise parameters are fully configurable

## Requirements

- Generates a grid of `grid_w × grid_h` vertices laid out in XZ, with Y set by `noise::fbm`
- Per-face normals computed from cross product of two triangle edges, normalized
- Per-face color determined by average height of 3 vertices, linearly interpolated across 4 `band_colors`
- No GL, XR, or Android dependencies — pure CPU geometry

## Allowed Dependencies

- `noise`
- `tri_mesh`

## Owning Package

`scene`
