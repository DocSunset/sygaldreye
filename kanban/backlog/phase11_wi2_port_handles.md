# WI-2: Port handles on node cards

Render colored port handle quads on the left (inputs) and right (outputs) edges of each node card in VrEditor.

## Deliverable

Extended `vr_editor.hpp` and `vr_editor.cpp` with `PortHandle` structs laid out from `port_schema` and rendered via `RgbaShader`.

## Acceptance

- Each node card shows colored quads for each non-draw_call port
- Handles positioned on left edge (inputs) and right edge (outputs)
- `sh/build.sh` passes
