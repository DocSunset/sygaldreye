# ui_nodes

Owning package: `scene` (ui)

UI widgets as graph nodes: ui_slider, ui_button, ui_pane. A hand ray +
trigger arrive through ordinary edges; values and draw calls flow out.
A slider is a source node; its drag state survives live edits.

## Ports
- Common inputs: x,y,z, width(/height), ray_pos (vec3), ray_rot (quat),
  trigger (slider).
- ui_slider: min, max, init; outputs value, render.
- ui_button: outputs pressed, hover, render.
- ui_pane: r,g,b,alpha; outputs render.
- Sources: hand nodes (host or XR) via edges — widgets never know which.
- Destinations: anything scalar (slider.value), spawner.trigger
  (button.pressed), the draw pass (render).
- Temporal couplings: hit state computed at tick from current ray; GL
  shader created lazily inside the first draw call (render thread).
- Intended seams: panels face +Z world-aligned; orientation becomes an
  input when needed. The editor's planned widget-subgraph rebuild
  composes these.

## Requirements
- Interaction math must be headless-testable (no GL outside draw
  closures) — enforced by ui_nodes_test.
- Degenerate ray quaternions fall back to identity.

## Allowed dependencies
vr_panel, rgba_shader, sygaldry_endpoints, Eigen.
