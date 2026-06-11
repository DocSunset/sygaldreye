# peer_core

The portable application: "the core that spins up and executes the graph
that spins up and executes the graphs… recursively to the leaf nodes"
(Travis, 2026-06-12). Host and Android (and the browser, next) are thin
shells over this — the same application as two build targets, differing
only in registered nodes, default startup graph, and frame pump. Exists
because every feature implemented per-shell drifted (the /screenshot and
/plugins gaps).

## Ports

- Inputs: `queue_param`, `queue_edit` (any thread, queue mappings);
  `registry` population + `init(Config)` from the shell; frame phases
  `begin_frame` / `pump_contexts` / `tick` / `collect_edits` /
  `fulfil_screenshot` called by the shell's loop on the render thread.
- Outputs: `graph()` (active graph: draw_calls, values), `probe`,
  `quit_requested`.
- Sources: HTTP requests (mongoose thread) — the full control surface:
  /graph /palette /plugins /param /values /camera /controller /play
  /screenshot /meta-graph /quit. Identical on every peer.
- Destinations: editor/spawner/fly_camera context seams (pump_contexts);
  "speaker" node receives /play params.
- Temporal couplings: fulfil_screenshot must run where the eye/frame
  image is readable (host: after draw; device: renderer post-resolve).
  begin_frame must precede tick (swap+params before processing).
- Intended seams: `on_graph_swapped` (device editor refresh);
  `Config.graphs_dir` for subgraph plugin packs; shells own registry
  population (peer capability = its node list).

## Requirements

- One implementation of every control-surface behavior; a route added
  here exists on every peer.
- /values serves a snapshot (never the live map — the render thread
  writes it mid-tick).
- Screenshot fulfilment is synchronous with a 2 s timeout; GET returns
  PNG bytes, POST writes a path.

## Allowed dependencies

component_registry, signal_graph, subgraph_node, http_server,
event_queue, editor_node, spawner_node, fly_camera_node, GLES3.
(stbi_write_png is extern — exactly one shell TU provides the
implementation.)

## Owning package

platform

## Restart-failure ledger hook

This component is where "what would need to have been changed" answers
about the control surface accumulate; see
kanban/backlog/live_update_gap.md.
