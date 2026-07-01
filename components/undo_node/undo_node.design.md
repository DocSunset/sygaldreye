# undo_node

The undo gesture (vr_editor_decomposition.md S3, from update_undo). A left-thumb
flick (thumb_x < −0.7, rising) restores the graph to the snapshot before the
last edit. Keeps a 1-deep history of the serialized live graph.

## Ports
- Sources: `thumb_x` (scalar) — left thumbstick.
- Destinations: the edit queue (a whole-graph edit, passed through verbatim).
- Temporal couplings: `set_context` each frame; captures a new baseline
  whenever the live graph changes.

## Requirements
- Resource-holder (unliftable): observes the graph it lives in.
- Rising-edge gesture; no repeat until released.

## Allowed dependencies
`editor_layout`, `sygaldry_endpoints`, `signal_graph`.

## Owning package
`scene`
