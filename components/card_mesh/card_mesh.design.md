# card_mesh

One editor card's geometry as a graph node
(`kanban/ready/vr_editor_decomposition.md` S4). The card subgraph's geometry
generator: a flat quad facing +z at `position`. Lifted over
`graph_source.positions` (keyed by id) → N cards. Geometry only; appearance is
the Surface from `vertex_color_mesh` downstream, drawn via the MeshArray
gather (the forest-route-1 pattern).

## Ports

- Inputs:
  - `position` — `in<Eigen::Vector3f>` (a CELL: lifted over the N×3 Span).
  - `width`, `height` — `normalled_in<float>` card extents.
- Outputs: `mesh` — `out<MeshPtr>`, a quad at `position`. Lifted outputs
  gather into a `MeshArray` for the card draw.
- Sources / Destinations: fed by the lift; drawn downstream.
- Temporal couplings: cached on (position,size) — regenerated only when a
  card moves.
- Intended seams: quad layout is the contract; richer card geometry (rounded
  panel, port rows) replaces this node without touching the lift.

## Requirements

- Cheap: one cached quad per card; no GL (geometry payload only).

## Allowed dependencies

`tri_mesh`, `render_payloads`, `sygaldry_endpoints`.

## Owning package

`scene`
