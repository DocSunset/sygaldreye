# aurora_curtain

Owning package: `scene` (visual)

One aurora curtain: a rippling additive-blended sheet. An aurora is a
subgraph of these (assets/graphs/aurora.json); the C++ monolith survives
only for the Android registry until APKs ship asset graphs.

## Ports
- Inputs: r,g,b, x_offset, phase, freq, speed, alt_base, alt_height,
  ripple_amp, depth (all sliders, all patchable).
- Outputs: render (DrawFn).
- Sources/Destinations: none beyond edges.
- Temporal couplings: ripple animates from tick time; GL (shader + grid
  via GlGeometry) initialized lazily on first tick on the render thread.
- Intended seams: per-curtain shader program is private; if N-curtain
  scenes ever profile poorly, share the program via a static cache without
  touching the node interface.

## Requirements
- Draws with additive blend, depth test off, and restores both.

## Allowed dependencies
gl_geometry, gl_program, sygaldry_endpoints, Eigen.
