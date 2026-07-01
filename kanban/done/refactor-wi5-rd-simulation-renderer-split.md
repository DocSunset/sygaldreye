# WI-5: Split reaction_diffusion into RDSimulation + RDRenderer

## Goal

`reaction_diffusion` already delegates GPU compute to `rd_gpu` internally, but this
seam is hidden. Expose it as two separate graph nodes so the RD texture can be consumed
by other nodes (e.g. as a foam mask for water_surface, a displacement map for terrain).

```
[RDSimulation] ──texture_port──► [RDRenderer] ──► render
```

## New Components

- `rd_simulation` — wraps `rd_gpu`, exposes inputs (Du, Dv, F, k, dt, steps_per_frame)
  and outputs a `port<"texture", GLuint>` (the ping-pong FBO color attachment)
- `rd_renderer` — receives a `port<"texture", GLuint>` input and the existing view
  matrix; renders the texture to screen using the current fragment shader

The existing `reaction_diffusion` node should be reimplemented as a thin subgraph
(instantiates both, wires the edge) or removed if the two separate nodes are sufficient.

## Acceptance Criteria

- `rd_simulation` and `rd_renderer` are separate components with separate CMakeLists
- A graph wiring `rd_simulation → rd_renderer` produces identical output to the current
  monolithic node
- The texture GLuint port is a first-class typed port in the sygaldry endpoint system
- `sh/build.sh` passes
- `reaction_diffusion_snapshot` still renders (either using the split subgraph or the
  retained monolithic node as a compatibility wrapper)

## Notes

`GLuint` is not a value-semantic type — the port carries the handle, not ownership.
Lifetime is owned by `RDSimulation`; `RDRenderer` must not delete the texture.
