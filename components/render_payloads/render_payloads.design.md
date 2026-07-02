# render_payloads

The GL-free edge payloads of the render-as-nodes path
(planning/render_as_nodes.md): `Surface` (appearance) and `Mesh` (drawable
geometry) flow shader-specific node → draw node; render_region reads them and
is the only place that says GL. Deliberately NO DrawDesc — a draw is the node
plus its wiring. Also hosts `common_shaders.hpp`, the shared GLSL sources
(unlit vertex-color, unlit uniform-color, MSDF text) so producers stop
pasting them and the boundary's content-keyed program cache dedupes them.

## Ports

- Inputs / Outputs: — (plain data: `Surface`, `Mesh`, `MeshArray`,
  `InstanceAttr`, `ShaderData`, `Uniform`, `ImageTex`, `Primitive`).
- Sources: `TriMeshData` (tri_mesh), `Span` (sygaldry_endpoints),
  `GpuTexture`.
- Destinations: render_region's caches key on payload identity + versions:
  `ShaderData` content, `TriMeshData::version`, `ImageTex::{key,version}`.
- Temporal couplings: borrowed pointers (`Span` data, `ImageTex::pixels`,
  `MeshArray::data`) must outlive the frame.
- Intended seams: `ImageTex::version` (0 ⇒ immutable pixels at a stable
  address; producers that regenerate pixels in place bump it);
  `Mesh::dynamic` as a buffer-usage hint.

## Requirements

- Header-only, GL-free (no GL types or constants leak to nodes).
- `Surface` carries pipeline state (blend/additive/depth/cull) as data.
- Kept in lock-step with eyeballs_node_abi's PortValue variant.

## Allowed dependencies

`sygaldry_endpoints`, `gpu_texture`, `tri_mesh`, Eigen.

## Owning package

`render`
