# ui_nodes

Owning package: `scene` (ui)

UI widgets as graph nodes: ui_slider, ui_button, ui_pane. A hand ray +
trigger arrive through ordinary edges; values and a declarative Mesh+Surface
flow out. A slider is a source node; its drag state survives live edits.

## Ports
- Common inputs: x,y,z, width(/height), ray_pos (vec3), ray_rot (quat),
  trigger (slider).
- ui_slider: min, max, init; outputs value, surface+mesh.
- ui_button: outputs pressed, hover, surface+mesh.
- ui_pane: r,g,b,alpha; outputs surface+mesh.
- Sources: hand nodes (host or XR) via edges — widgets never know which.
- Destinations: anything scalar (slider.value), spawner.trigger
  (button.pressed), a `draw` node (surface+mesh).
- Temporal couplings: hit state computed at tick from current ray.
- Intended seams: panels face +Z world-aligned; orientation becomes an
  input when needed. The editor's planned widget-subgraph rebuild
  composes these.

## Requirements
- ABI v8: render is a declarative Mesh+Surface (blended unlit, per-vertex
  color), never GL. Interaction math is headless-testable — enforced by
  ui_nodes_test.
- Degenerate ray quaternions fall back to identity.

## Allowed dependencies
sygaldry_endpoints, render_payloads, tri_mesh, eyeballs_node_abi (plugin
export only), Eigen.
