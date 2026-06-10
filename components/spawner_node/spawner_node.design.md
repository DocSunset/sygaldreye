# spawner_node

Owning package: `scene` (editor)

Adds a node of `type` to the enclosing graph on each rising trigger edge.
The primitive that lets graph-built UI extend the graph.

## Ports
- Inputs: trigger (slider; rising edge fires), type (text param).
- Outputs: none (edits flow via take_edit; becomes a `graph_edit` event
  output when event edges land).
- Sources: `set_context(graph, registry)` platform pump.
- Destinations: `take_edit()` platform collection.
- Temporal couplings: same as editor_node.
- Intended seams: wire ui_button.pressed → trigger; palettes in JSON.

## Requirements
- Unknown type or empty type must no-op.
- Generated ids must not collide (type + timestamp).

## Allowed dependencies
signal_graph, component_registry, sygaldry_endpoints.
