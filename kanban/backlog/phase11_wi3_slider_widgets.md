# WI-3: Inline parameter sliders on node cards

For each scalar input port, draw a horizontal slider widget that the user can grab with the right controller trigger.

## Deliverable

`SliderWidget` in `vr_editor.hpp`; layout in `on_graph_changed`; interaction in `update`; rendering in `draw`.

## Acceptance

- Sliders visible on node cards for scalar inputs
- Controller ray + trigger updates slider value and calls `set_scalar_in`
- `sh/build.sh` passes
