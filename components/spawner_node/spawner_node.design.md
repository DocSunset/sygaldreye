# spawner_node

Owning package: `scene` (editor)

Adds a node of `type` to the enclosing graph on each rising trigger edge.
The primitive that lets graph-built UI extend the graph.

## Ports
- Inputs: trigger (slider; rising edge fires), type (text param).
- Outputs: none (edits push onto the shared edit queue).
- Sources: `set_context(graph, registry, edit queue)` injected by
  peer_core's pump_contexts (the generic ABI v9 context hook this arc).
- Destinations: edit ops onto peer_core's edit queue, applied by
  collect_edits.
- Temporal couplings: context injected before tick each frame.
- Intended seams: wire ui_button.pressed → trigger; palettes in JSON.

## Requirements
- Unknown type or empty type must no-op.
- Emits a structured `add_node` op; apply_edit_op assigns the smallest
  free `<type>_N` id (no collisions after deletions).

## Allowed dependencies
signal_graph, component_registry, sygaldry_endpoints, event_queue,
editor_layout (context type + json_escape).
