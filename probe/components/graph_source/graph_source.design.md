# graph_source

The editor's meta seam (`kanban/ready/vr_editor_decomposition.md`): the ONE
node that reads the enclosing graph and publishes its model as data. Replaces
`EditorNode::set_context` raw-pointer injection — with this node the editor
stops being a monolith and becomes graph content (a lifted card-subgraph)
fed by this source. "The editor is meta: it needs the WHOLE graph as data."

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
- Sources: the enclosing `Graph`, injected each frame by the shell
  (`PeerCore::pump_contexts`) — the graph can't be a port, it contains this
  node. The same seam the editor/spawner used.
- Destinations: the card subgraph (lifted) and the gesture nodes downstream.
- Temporal couplings: `set_context` must run before `tick` each frame;
  `migrate_graph` keeps this instance (and its card-position overrides) alive
  across the edits it helps produce.
- Intended seams: card layout (the per-id position override map) is updated
  by edit ops; alternative layouts replace the default grid without touching
  the consumers.

## Requirements

- Resource-holder (`lift_kind == resource_holder`): unliftable — it reads the
  graph it lives in.
- Stable keys: identity follows the node id (FNV-1a hash), not the slot.

## Allowed dependencies

`sygaldry_endpoints`, `signal_graph`.

## Owning package

`scene`
