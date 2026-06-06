# WI-4: Wire rendering

Draw bezier curves between connected port handles for each edge in the graph.

## Deliverable

`current_edges_` stored in `VrEditor`; `draw_wire()` helper draws cubic bezier line segments using `RgbaShader`.

## Acceptance

- Edges in graph are drawn as curved lines between handle positions
- Color matches port kind
- `sh/build.sh` passes
