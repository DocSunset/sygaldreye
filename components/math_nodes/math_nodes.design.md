# math_nodes

Owning package: `scene` (primitives)

The modular-synth primitive vocabulary: lfo, scale, add, mul, const, sub,
div, phasor, smooth, split3, join3. Larger behaviours should be subgraphs
of these, not new C++ nodes.

## Ports
- Inputs/Outputs: per node; all scalar sliders except split3 (vec3 in) and
  join3 (vec3 out). `add` doubles as a constant/fan-out helper
  (a unwired, b=value) — prefer `const` now that it exists.
- Sources/Destinations: none — pure functions of inputs (+ time).
- Temporal couplings: phasor and smooth keep internal state (phase,
  one-pole memory) keyed to the tick time; both survive live edits via
  migrate_graph, which is the point.
- Intended seams: this header is the canonical place for new scalar/vector
  primitives; keep each node a few lines.

## Requirements
- Header-only, allocation-free, no platform deps.
- div-by-zero yields 0; scale with zero span yields out_min.

## Allowed dependencies
sygaldry_endpoints, Eigen (split3/join3 only).
