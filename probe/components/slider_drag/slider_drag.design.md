# slider_drag

The scalar-slider drag (vr_editor_decomposition.md S3, from update_sliders).
With the trigger held, the tip's x over the single nearest slider track maps to
a value in [min,max] → a `set_param` edit op. One slider at a time (y-nearest,
half a port row), matching the monolith.

## Ports
- Sources: `pos` (vec3), `rot` (quat), `trigger` (scalar) — right controller.
- Destinations: the edit queue (`set_param`), via `GestureContext`.
- Temporal couplings: `set_context` each frame; ticks every frame.
- Intended seams: track width / tolerances match the monolith.

## Requirements
- Resource-holder (unliftable): observes the graph it lives in.
- Value = clamp((tip.x − track.x + w/2)/w) mapped to the port's [min,max].

## Allowed dependencies
`editor_layout`, `sygaldry_endpoints`, `signal_graph`.

## Owning package
`scene`
