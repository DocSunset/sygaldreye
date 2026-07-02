# tri_mesh

CPU triangle-mesh payload: `TriVertex` (position/normal/color) and
`TriMeshData` (vertices + indices + version stamp). GL-free — the GPU copy is
owned by `render_region`, which caches by `TriMeshData*` and re-uploads when
`version` changes. The former GL `TriMesh` class was deleted (unused since the
render-as-nodes migration).

## Ports

- Inputs: — (plain data)
- Outputs: `version` — a globally unique stamp, assigned at construction and
  re-stamped by `touch()`; consumers use it to detect content changes without
  comparing vertices, and address reuse can never alias a stale stamp.
- Sources: —
- Destinations: `render_region`'s geometry cache keys on
  (`TriMeshData*`, `version`).
- Temporal couplings: producers must `touch()` after in-place mutation and
  before the frame's draw is issued (same-thread tick → issue).
- Intended seams: none — a POD payload.

## Requirements

- Interleaved layout: position (vec3), normal (vec3), color (vec4).
- Version stamps are process-unique and monotonic (atomic counter).

## Allowed dependencies

- Eigen only.

## Owning package

`render`
