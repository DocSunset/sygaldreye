# scene_graph

Transform hierarchy for composing multi-part objects. Each node holds a local transform and a parent reference; world transforms are resolved lazily by walking to the root.

## Ports

**Inputs**
- `add_node(parent)` — add a node with an optional parent, returns a stable `NodeId`
- `set_transform(id, t)` — update a node's local transform and invalidate its subtree

**Outputs**
- `world_transform(id)` — composed world-space transform for a node (cached)

**Sources / Destinations / Temporal couplings**
- None. Pure transform math; no rendering or XR dependency.

**Intended seams**
- `NodeId` is a plain integer; callers (renderable objects) store it and query the graph each frame.

## Requirements

- Lazy world-transform computation with a mutable cache; no recomputation until a node in the ancestor chain is modified.
- `invalidate(id)` clears the cache for `id` and all its descendants.
- Nodes are never removed; indices are stable for the session lifetime.
- No GL, XR, or Android dependencies.

## Allowed dependencies

- Eigen (geometry / linear algebra)

## Owning package

`scene`
