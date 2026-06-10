# editor_node

Owning package: `scene` (editor)

The graph editor as a node in the graph it edits. Wraps VrEditor.

## Ports
- Inputs: left/right pos (vec3) + rot (quat), trigger_left/right,
  grip_right, thumb_x/y (sliders) — normally edge-fed from hand nodes.
- Outputs: render (DrawFn).
- Sources: `set_context(graph, registry)` — platform-injected each frame
  before tick (see edge_executor design for the ABI-context replacement).
- Destinations: `take_edit()` — platform collects new-graph JSON after
  tick and stages it as the pending graph.
- Temporal couplings: context before tick; edit collection after; card
  model refresh when the graph pointer changes (migration keeps this
  instance alive across its own edits — undo state survives).
- Intended seams: planned decomposition into ui_* widget subgraphs; this
  node is scaffolding until then.

## Requirements
- Must function with stale/no context by doing nothing.
- Editing must never reset its own undo state (migration contract).

## Allowed dependencies
vr_editor, text_mesh, sygaldry_endpoints, signal_graph, component_registry.
