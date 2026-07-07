# editor_wires

The live graph's edges as a wire Span (vr_editor_decomposition.md, from
VrEditor::collect_wires): an N×10 row {from.xyz, to.xyz, rgba} per edge, for a
wire_mesh node. Reads the same editor_layout the cards and gestures do, so a
wire lands on the handles; colour is the producer port's kind.

## Ports
- Outputs: `wires` (Span, N×10).
- Sources: the live graph + overrides, via `GestureContext`.

## Requirements
- Resource-holder (unliftable): observes the graph it lives in.
- Edges whose endpoints aren't laid out (missing handles) are skipped.

## Allowed dependencies
`editor_layout`, `sygaldry_endpoints`, `signal_graph`.

## Owning package
`scene`
