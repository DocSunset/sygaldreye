# aurora_curtain

Owning package: `scene` (visual)

One aurora curtain: a rippling additive-blended sheet. An aurora is a
subgraph of these (assets/graphs/aurora.json); the C++ monolith survives
only for the Android registry until APKs ship asset graphs.

## Ports
- Inputs: r,g,b, x_offset, phase, freq, speed, alt_base, alt_height,
  ripple_amp, depth (all sliders, all patchable).
- Outputs: surface + mesh (render-as-nodes), consumed by a `draw` node.
- Sources/Destinations: none beyond edges.
- Temporal couplings: ripple animates from tick time; GL lives in
  render_region, which draws the emitted mesh.
- Intended seams: per-curtain shader program is private; if N-curtain
  scenes ever profile poorly, share the program via a static cache without
  touching the node interface.

## Requirements
- Draws with additive blend, depth test off, and restores both.

## Allowed dependencies
gl_geometry, gl_program, sygaldry_endpoints, Eigen.
