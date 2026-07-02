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

Not a node; a pure library (`build_layout`, `cached_layout`, `edge_endpoints`,
`id_key`, `default_card_pos`, `json_escape`). Also defines `GestureContext` —
the "editor"-kind host context (ABI v9 `set_host_context`) carrying the live
graph, edit queue, position overrides, sorted type list, registry, and the
shared `LayoutCache` with its generation stamps.

- Inputs: a `Graph` + the per-id position-override map; for `cached_layout`,
  a `GestureContext` (its `graph_gen`/`overrides_gen` key the memo).
- Outputs: a `Layout` (cards, each with input/output handles and sliders).
- Sources/Destinations/Temporal couplings: the host (peer_core) owns the
  cache and bumps `graph_gen` on swap + in-place param writes; wire_drag
  bumps `overrides_gen` on card drags. Without a cache in the context,
  `cached_layout` rebuilds per call (tests).
- Intended seams: any node consuming kind "editor" via `set_host_context`
  joins the editor without host changes.

## Requirements

- Geometry byte-for-byte matches the monolith (kCardW 0.4, kBaseCardH 0.08,
  kPortRowH 0.018, handle offset 0.01, scalar inputs get a slider at card x).
- Slider values seeded from each node's live serialized params.

## Allowed dependencies

`signal_graph`, `port_schema_reader`, `sygaldry_endpoints`.

## Owning package

`scene`
