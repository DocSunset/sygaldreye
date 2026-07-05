# event_queue

The canonical `queue` mapping from the edge-executor design: events that
cross a thread boundary go through an MPSC queue that never drops and never
duplicates. One implementation, reused everywhere a "pending/take" pattern
used to be hand-rolled (editor/spawner `take_edit()`, the shells' param
queues).

## Ports

Library component (not yet a node — reify when the executor auto-inserts
queue mappings at inferred event boundaries).

- Inputs: `push(T)` from any thread.
- Outputs: `drain()` — all pending events in arrival order, consumer thread.
- Intended seams: `T` is any movable payload (graph-edit JSON, param events).

## Requirements

- MPSC; never drops; never duplicates; arrival order preserved.
- Not yet RT-safe (mutex + deque) — the audio region will need an SPSC
  ring variant (`ring` mapping) instead; do not reuse this there.

## Allowed dependencies

Standard library only.

## Owning package

scene (graph infrastructure)
