# flat_mesh

A render-as-nodes graph node (ABI v8; renamed from the flat_shader
component so node name = component name) for UNLIT, FLAT-shaded
geometry. The GLSL `flat` qualifier fills each triangle with its
provoking-vertex color — no interpolation, no lighting. The distinct shading
the lit nodes don't offer (cube_node/color_mesh are lit single-color;
vertex_color_mesh is lit smooth). Good for faceted/low-poly looks and
debug-colored meshes. Wraps raw geometry into Surface + Mesh for a `draw`
node; GL lives only in render_region.

## Ports

- Inputs: `geometry` (MeshPtr — vertices carry aColor); `positions` (Span,
  N×3 instance offsets; unwired ⇒ one at origin); `tint_r`/`g`/`b` (params,
  a multiplicative tint, default white = passthrough).
- Outputs: `surface` + `mesh` — the two payloads a `draw` node consumes.
- Destinations: a `draw` node.
- Temporal couplings: none (shader built lazily on first tick, CPU-side).
- Intended seams: instancing is rank-polymorphic via `positions` (one mesh →
  a field of flat-shaded copies).

## Requirements

- Per-vertex RGBA color at attribute location 2; `aOffset` at location 3.
- GLSL `flat` interpolation: provoking vertex color fills the whole triangle.
- No lighting computation. render_region injects uMVP.

## Allowed dependencies

- `eyeballs_node_abi`, `render_payloads`, `tri_mesh` (tests), Eigen

## Owning package

`render`
