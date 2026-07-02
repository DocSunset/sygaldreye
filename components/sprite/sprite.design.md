# sprite

Shader-specific node for camera-facing billboards: owns a unit quad and a
facing shader that builds each corner from the render-time camera basis. Fed
an N×3 positions Span it draws N soft round sprites in one instanced,
blended draw — particles/billboards with no GL in the node.

## Ports

- Inputs: `positions` (Span, N×3; unwired ⇒ one at origin); `size`,
  `r`/`g`/`b` (params).
- Outputs: `surface` (blend on, depth_write off, no cull) + `mesh` (shared
  unit quad + the positions Span at divisor 1).
- Sources: any Span producer (particle_system's pool, scatter).
- Destinations: a `draw` node; render_region injects uCameraRight/uCameraUp
  per eye and uploads the Span once per frame.
- Temporal couplings: the Span is borrowed for the frame.
- Intended seams: camera basis comes from the boundary, so the same node
  works on host (fly camera) and per-eye on Quest.

## Requirements

- Quad built once (stable version ⇒ uploaded once, then bound); soft-disc
  alpha falloff in the fragment shader.
- Blended, non-occluding (depth test on, depth write off).

## Allowed dependencies

`eyeballs_node_abi`, `render_payloads`, `tri_mesh`, Eigen.

## Owning package

`render`
