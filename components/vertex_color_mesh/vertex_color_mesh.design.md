# vertex_color_mesh

Shader-specific node for lit, per-vertex-colored geometry (terrain,
reaction-diffusion grids, editor quads): the mesh carries its colors in
aColor; this node applies one directional Blinn-Phong sun and emits
`Surface` + `Mesh`.

## Ports

- Inputs: `geometry` (MeshPtr); `sun_dir`/`sun_color` (vec3; unwired ⇒
  default downward warm sun); `sun_intensity` (param).
- Outputs: `surface` + `mesh` (dynamic hint set — producers animate it).
- Sources: mesh producers that mutate in place and `touch()` (chladni, RD,
  deformer chains); sky_dome's sun outputs.
- Destinations: a `draw` node; render_region re-uploads only when the
  geometry's version changed (per frame at most, never per eye).
- Temporal couplings: upstream producer must `touch()` before the frame's
  issue; MeshPtr borrowed for the frame.
- Intended seams: sun inputs are wireable (day/night from sky.json).

## Requirements

- Fixed uniforms (uSunDir/uSunColor/uSunIntensity + injected uViewPos/uMVP);
  aPos/aNormal/aColor layout. No GL in the node.

## Allowed dependencies

`eyeballs_node_abi`, `render_payloads`, Eigen.

## Owning package

`render`
