# dwell_delete

The dwell-to-delete gesture (vr_editor_decomposition.md S3, from update_dwell).
Grip held + a 1 s dwell of the tip inside a card (or near an edge midpoint)
emits `remove_node` / `remove_edge`. Card wins over edge, as in the monolith.

## Ports
- Sources: `pos` (vec3), `rot` (quat), `grip` (scalar) — right controller.
- Destinations: the edit queue (`remove_node` / `remove_edge`).
- Temporal couplings: dt from successive `time_s`; must tick every frame.
- Intended seams: dwell time (1 s) + hit radii match the monolith.

## Requirements
- Resource-holder (unliftable): observes the graph it lives in.
- Deliberate: requires grip; bare hover never deletes.

## Allowed dependencies
`editor_layout`, `sygaldry_endpoints`, `signal_graph`.

## Owning package
`scene`
