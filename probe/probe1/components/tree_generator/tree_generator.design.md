# tree_generator

Procedurally generates a low-poly tree as a `TriMeshData` (trunk + branches as faceted cylinders) and a `std::vector<BillboardInstance>` leaf clusters.

## Ports

### Inputs
- `TreeParams` passed to `generate_tree()` — controls all geometry parameters.

### Outputs
- `TreeMesh::branches` — `TriMeshData` of all cylinder segments (flat-shaded).
- `TreeMesh::leaves` — `BillboardInstance` list at branch tips.

### Sources
None.

### Destinations
- `branches` consumed by `TriMesh` / renderer.
- `leaves` consumed by `BillboardQuad` / renderer.

### Temporal Couplings
None — purely functional.

### Intended Seams
`BranchParams` fields (depth, count, scale, noise, seed) allow varied tree archetypes without modifying source.

## Requirements

- R1: Each call to `generate_tree` is pure and deterministic for a given `TreeParams`.
- R2: Cylinder segments have flat (per-face) normals; all normals unit length.
- R3: Leaf `BillboardInstance` entries appear only at depth ≥ `max_depth` (tips).
- R4: Node count: trunk + sum of branch_count^d for d=1..max_depth.
- R5: No GL, XR, or Android dependencies — CPU only.

## Allowed Dependencies

- `noise`
- `tri_mesh`
- `billboard_quad`
- Eigen

## Owning Package

`scene`
