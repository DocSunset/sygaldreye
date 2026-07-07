# wire_drag

The grip-edge connect/move FSM (vr_editor_decomposition.md S3), decomposed
from `vr_editor_interactions.cpp::update_drag`. On grip-down it grabs the
nearest output handle under the tip; on grip-up it releases on the nearest
kind-compatible input handle → an `add_edge` edit op. No output handle under
the tip ⇒ it grabs the card body and moves it while gripped, writing the
shared per-id position overrides graph_source reads.

## Ports

- Inputs: —
- Sources: `pos` (vec3), `rot` (quat), `grip` (scalar) — the right controller,
  via the hand node's edges.
- Outputs: — (edits flow out of band)
- Destinations: the edit queue (`add_edge` ops) + graph_source's overrides
  (card move) — injected via `GestureContext`.
- Temporal couplings: `set_context` each frame before tick; grip rising/falling
  edges drive the FSM, so it must tick every frame to see both.
- Intended seams: hit radii (0.02 grab, 0.03 connect) match the monolith.

## Requirements

- Resource-holder (unliftable): observes the graph it lives in.
- Connect only kind-compatible ports (`port_types::connection_legal`).
- Card move follows the tip with the grab offset; persists in overrides.

## Allowed dependencies

`editor_layout`, `port_types`, `sygaldry_endpoints`, `signal_graph`.

## Owning package

`scene`
