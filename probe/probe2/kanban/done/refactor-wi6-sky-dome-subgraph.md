# WI-6: Decompose sky_dome into SkyShader + SunDisc + StarField subgraph

## Goal

`sky_dome` mixes three independent rendering concerns inside one monolithic node:
sky gradient, sun disc, and star field. Decompose into three focused nodes that
compose as a subgraph. This makes `sun_dir` a first-class graph edge, allowing
`water_surface` (and any future node) to receive consistent sun direction without
manual duplication.

```
[SkyParams] ──► [SkyShaderNode]  ──► render
           ──► [SunDiscNode]     ──► render
           ──► [StarFieldNode]   ──► render
           
[SkyShaderNode] ──sun_dir──► [WaterSurface / OceanShader]
```

## New Components

- `sky_shader` — sky gradient dome; inputs: sun_elevation, azimuth; outputs: render,
  sun_dir (Eigen::Vector3f), sun_color (Eigen::Vector4f)
- `sun_disc` — bright billboard at sun position; inputs: sun_dir (Eigen::Vector3f);
  output: render
- `star_field` — randomised point sprites; inputs: sun_elevation (fade in/out),
  star_count, radius; output: render

The current `sky_dome` node can be retained as a compat wrapper that instantiates
all three and forwards their outputs.

## Acceptance Criteria

- Three new components, each with CMakeLists, hpp/cpp, and at minimum a smoke test
- Composing the three nodes in a graph produces visually identical output to the
  monolithic sky_dome
- `sun_dir` output from `sky_shader` is a usable `port<"sun_dir", Eigen::Vector3f>`
- `sky_dome_snapshot` still builds and renders
- `sh/build.sh` passes

## Notes

`SphereGeometry` / `sphere_mesh` already exists for the dome mesh — reuse it.
The star field is currently randomised with a fixed seed; preserve that behavior.
