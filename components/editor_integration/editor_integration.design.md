# editor_integration

Test-only: the editor gesture → edit-op → graph round-trip
(vr_editor_decomposition.md verification). Each gesture node, fed controller
poses against a real registry graph, emits the same structured edit the
monolith produced; `apply_edit_op` + `parse_graph` realise it. No GL.

## Ports
Not a node — a gtest binary asserting the connect / set_param / remove_node /
add_node round-trips end-to-end.

## Requirements

- Each gesture (wire-drag connect, slider-drag set_param, dwell-delete
  remove_node, palette-poke add_node) driven by synthetic controller
  poses against `editor_layout`'s layout emits exactly ONE structured
  edit op.
- `apply_edit_op` + `parse_graph` realise each op as the intended graph
  change (edge added, param set, node removed/added).
- Weird ids (quote/backslash) escape at the gesture and unescape in the op.
- PeerCore seams: the v9 host context reaches a consumer INSIDE a subgraph;
  a `set_param` op takes the in-place path (active Graph pointer survives).
- Pure data flow: no GL work (headless PeerCore only; its screenshot stub
  is provided by the test TU, its HTTP server binds an ephemeral port).

## Allowed dependencies
`wire_drag`, `slider_drag`, `dwell_delete`, `palette`, `editor_layout`,
`component_registry`, `signal_graph`, `math_nodes`, `peer_core` (+ GLESv2
for peer_core's readback symbol).

## Owning package
`scene`
