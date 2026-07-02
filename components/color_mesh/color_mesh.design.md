# color_mesh

Shader-specific node: wraps raw geometry (MeshPtr) and a color into a
`Surface` (simple lit, single-color program) + `Mesh` — the two payloads a
`draw` node consumes. The first render-as-nodes shader node; rank-polymorphic
(one node draws a cube or a forest).

## Ports

- Inputs: `geometry` (MeshPtr); `positions` (Span, N×3 instance offsets;
  unwired ⇒ one at origin); `color` (vec4 override) or `r`/`g`/`b` params.
- Outputs: `surface` + `mesh`.
- Sources: a geometry generator (mesh_grid/mesh_box/…); optionally a Span
  producer (scatter).
- Destinations: a `draw` node; render_region compiles the shader once
  (content-keyed) across all instances.
- Temporal couplings: emitted Span/MeshPtr are borrowed for the frame.
- Intended seams: an N-row `positions` Span becomes an instanced draw
  ("excess rank → lift" at the boundary).

## Requirements

- Fixed uniform set (`uColor`) + attribute layout (aPos/aNormal/aColor,
  aOffset at divisor 1) — correct-by-construction bundles, no bind-by-name.
- No GL; shader built lazily as data on first tick.

## Allowed dependencies

`eyeballs_node_abi`, `render_payloads`, Eigen.

## Owning package

`render`
