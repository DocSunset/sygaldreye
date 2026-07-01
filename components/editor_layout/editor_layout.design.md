# editor_layout

The editor's spatial model as ONE shared function (vr_editor_decomposition.md
S3/S5). Given the live graph + per-id card-position overrides, lays out every
card, port handle and scalar slider in world space — ported verbatim from the
VrEditor monolith (`vr_editor.cpp` default grid + `vr_editor_handles.cpp`
columns/pitch). The decomposed gesture nodes and the card subgraph share this
so they hit-test and render identical geometry.

Identity (node id + port name) travels WITH the geometry, so a gesture node
that observes the graph turns a nearest-hit straight into an edit op — the
simplest identity resolution, no text-array payload.

## Ports

Not a node; a pure library (`build_layout`, `edge_endpoints`, `id_key`,
`default_card_pos`).

- Inputs: a `Graph` + the per-id position-override map.
- Outputs: a `Layout` (cards, each with input/output handles and sliders).
- Sources/Destinations/Temporal couplings/Intended seams: —

## Requirements

- Geometry byte-for-byte matches the monolith (kCardW 0.4, kBaseCardH 0.08,
  kPortRowH 0.018, handle offset 0.01, scalar inputs get a slider at card x).
- Slider values seeded from each node's live serialized params.

## Allowed dependencies

`signal_graph`, `port_schema_reader`, `sygaldry_endpoints`.

## Owning package

`scene`
