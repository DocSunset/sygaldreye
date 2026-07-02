# Card subgraph decomposition (plan S4/S5, undone)

`editor_layout` is the monolith's core transplanted as a C++ library:
card geometry, handle columns, slider layout, grid, and palette
placement are compile-time constants consumed by 7 nodes holding raw
`Graph*`; `card.json` is a 1-node wrapper around `card_mesh`. The
plan's S4/S5 (lifting_and_editor_decomposition.md) — card subgraph
builds panel + per-port handle lift + sliders + label; palette rows =
per-type lift — remains undone. Restyling a card or moving the palette
requires a rebuild (live_update_gap.md's exact complaint), and
graph_source.design.md's "the ONE node that reads the enclosing graph"
is false ×7. reports/audit_conformability_editor_arc.md §1.
