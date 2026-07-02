# graph_source

The editor's meta *source* (`kanban/ready/vr_editor_decomposition.md`): reads
the enclosing graph and publishes its model as data for the lifted card
subgraph. It is NOT the only graph reader — every context-consuming editor
node (gesture nodes, card_widgets_mesh/card_labels_mesh/editor_wires,
undo_node) receives the same live graph through the generic host-context
seam (ABI v9 `set_host_context`, kind "editor", built by
`PeerCore::pump_contexts`). graph_source is the one that turns it into
lift-ready Spans.

## Ports

- Inputs: —
- Outputs:
  - `keys` — `out<Span>`, N×1 stable per-node id hash. The KEY source for
    the keyed card lift: a card subgraph lifted over `positions` keys on
    THIS, not the position cell, so a dragged card keeps its identity across
    a reorder or a live edit.
  - `positions` — `out<Span>`, N×3 card world positions (the lifted-cell
    source → `card.position`).
  - `count` — `out<float>`, N (node count).
- Sources: the enclosing `Graph` + the shared per-id position-override map,
  injected each frame through the editor host context (the graph can't be a
  port — it contains this node).
- Destinations: the card subgraph (lifted) and downstream layout consumers.
- Temporal couplings: context must arrive before `tick` each frame;
  `migrate_graph` keeps this instance alive across the edits it helps
  produce.
- Intended seams: the position-override map is written DIRECTLY by
  wire_drag's card-move (shared host-owned state, not edit ops) — so drag
  positions are not serialized, not undoable, and invisible to remote peers.
  Making card positions graph content is tracked in
  `kanban/backlog/card_positions_as_graph_content.md`. Alternative layouts
  replace the default grid without touching the consumers.

## Requirements

- Resource-holder (`lift_kind == resource_holder`): unliftable — it reads the
  graph it lives in.
- Stable keys: identity follows the node id (FNV-1a hash), not the slot.
  Hash + default grid are `editor_layout::id_key` / `default_card_pos` —
  ONE definition, shared with the hit-testing nodes.

## Allowed dependencies

`sygaldry_endpoints`, `signal_graph`, `editor_layout`.

## Owning package

`scene`
