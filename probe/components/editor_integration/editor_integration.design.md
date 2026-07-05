# editor_integration

Test-only: the editor gesture → edit-op → graph round-trip
(vr_editor_decomposition.md verification). Each gesture node, fed controller
poses against a real registry graph, emits the same structured edit the
monolith produced; `apply_edit_op` + `parse_graph` realise it. No GL.

## Ports
Not a node — a gtest binary asserting the connect / set_param / remove_node /
add_node round-trips end-to-end.

## Allowed dependencies
`wire_drag`, `slider_drag`, `dwell_delete`, `palette`, `editor_layout`,
`component_registry`, `signal_graph`, `math_nodes`.

## Owning package
`scene`
