# host_app

Owning package: `platform` (desktop variant)

Desktop platform shell. Owns the registry, HTTP control surface, frame
pump, and final draw dispatch. Everything else lives in the graph.

## Ports
- Inputs: `--port`/`--headless` (via spectator main); HTTP requests
  (`/graph`, `/param`, `/camera`, `/controller`, `/values`, `/palette`,
  `/screenshot`, `/quit`).
- Outputs: rendered frames into the current framebuffer; PNG screenshots;
  HTTP JSON responses.
- Sources: `camera.pv` from the graph value store (any `fly_camera` node
  named `camera`); edit ops via peer_core's edit queue (gesture nodes /
  edit_sink).
- Destinations: per-node `deserialize` (param queue applied on the render
  thread); editor-context injection is peer_core's pump_contexts (the
  graph_source/edit_sink/gesture seam; generic ABI v9 hook this arc);
  `inputs.aspect` on fly_camera nodes.
- Temporal couplings: pending graph swap + param queue apply happen at
  frame start, before tick; edits collected after tick; screenshots
  fulfilled after draw. GL context must be current before `init()` (node
  constructors may compile shaders).
- Intended seams: node registration list (`init`), HTTP route table.
  (The context-injection type list dies with ABI v9's generic
  editor-context hook, this arc.)

## Requirements
- Live graph overwrite must not reset node state (migrate_graph).
- All HTTP mutation is queued and applied on the render thread.
- Headless and windowed modes share `frame()` unchanged.

## Allowed dependencies
peer_core, component_registry, signal_graph, subgraph_node, http_server,
fly_camera, fly_camera_node, hand_node, spawner_node, math_nodes,
net_nodes, ui_nodes, text_label, registered visual/scene/editor nodes,
stb, GLFW.
