# palette

The node palette (vr_editor_decomposition.md S3, from vr_editor.cpp palette
block). A poke (trigger rising, tip in the panel) on a type row emits an
`add_node` op; row 0 flips pages. The sorted type list comes from the registry
via the gesture context.

## Ports
- Sources: `pos` (vec3), `rot` (quat), `trigger` (scalar) — right controller;
  the registry type list + edit queue via the editor host context (ABI v9).
- Outputs: `page` (scalar) — the current page, wired to `palette_mesh.page`
  so the drawn rows are the poked rows.
- Destinations: the edit queue (`add_node`, type json-escaped).
- Temporal couplings: context injected each frame; trigger rising edge fires
  once.
- Intended seams: panel position/size + 15 rows/page match the monolith.

## Requirements
- Resource-holder (unliftable): observes the graph it lives in.
- Paging wraps; out-of-range rows are inert.

## Allowed dependencies
`editor_layout`, `sygaldry_endpoints`, `signal_graph`.

## Owning package
`scene`
