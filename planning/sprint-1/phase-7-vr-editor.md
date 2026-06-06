# Phase 7: In-VR Structural Editor

## Goal

A fully in-headset editor for the node graph. Instantiate nodes, wire ports, reposition
cards, delete connections — without taking off the headset or touching a keyboard.
The editor is itself a set of nodes in the graph and can be reconfigured via `/meta-graph`.

## Editor as a meta-layer

The editor runs as a fixed overlay, always present, separate from the user-editable
scene graph. It manages the scene graph but is not part of it. Its own structure can be
modified via the out-of-band `/meta-graph` POST endpoint (see Phase 5), not via `/graph`.

## VR UI Primitives (built first, reused everywhere)

These are small reusable components that the editor — and any future in-world UI — builds on:

- **`vr_panel`**: a flat quad in world space with a 2D canvas (texture-backed); handles
  ray-intersection testing
- **`ray_selector`**: emits a ray from a controller's pose; reports hit results against
  registered panels and handles
- **`slider_widget`**: 2D slider rendered onto a panel canvas; reads/writes a float ref
- **`text_label`**: renders a string onto a panel canvas using `text_mesh`
- **`port_handle`**: small sphere/disc rendered at a port position; grabbable; typed

Phase 2's VR slider panel is built from these primitives. Phase 7 reuses them.

## Editor components

### Palette panel

- Floating panel anchored near the non-dominant hand
- Scrollable list of all registered node types (from component registry)
- Tap a type to spawn a new node card in front of the user

Future: replace with a pie/marking menu on thumbstick flick; or speech ("add water surface").

### Node cards

- One card per node instance in the graph, positioned freely in 3D space
- Displays: node type name, instance ID
- Left column: input port handles, one per `inputs` member
- Right column: output port handles, one per `outputs` member
- Center: inline `slider_widget` for each scalar input (Phase 2 primitives reused)
- Grabbable: pinch-and-drag to reposition

### Wire rendering

- Bezier curve between connected output and input port handles
- Color-coded by port type (scalar, audio, texture, draw_call, pose)
- Rendered as a thin tube or line strip

### Drag-to-connect

1. Grip an output port handle
2. A "ghost wire" follows the controller
3. Hover over a compatible input handle — highlight if types match, red if not
4. Release: edge added to graph; wire rendered

Type checking: port types must match. Rate mismatch prompts insertion of a rate-converter
node.

### Delete

- Point at a wire or card, hold trigger for 1s → delete with haptic confirmation
- Undo: POST previous graph snapshot to `/graph`

## Self-reference

Because the editor's own panels are `vr_panel` nodes in the meta-graph, they can be
repositioned, resized, or replaced via `/meta-graph` POST. A sufficiently determined
LLM assistant could redesign the editor layout without recompiling.

## Key Design Decisions

**Editor state is separate from scene state.** Node card positions are stored in the
meta-graph, not in the scene graph JSON. POST `/graph` does not move or remove node cards.

**Palette first, pie menu later.** The palette panel is simpler to implement and easier
to debug. Pie/marking menus and gesture-based spawning are enhancements once the basic
flow works.

**Speech as a first-class input channel (Phase 3 + 7).** "Add a water surface" spoken
while the editor is open → Phase 3 pipeline → Claude → POST `/graph`. The editor
responds by creating a new node card. The VR UI and the speech channel operate in parallel
on the same graph endpoint.

## Dependencies

Phase 5 (universal signal graph) — editor reads/writes the live graph.
Phase 2 (networking) — node cards update when remote POSTs change the graph.
Phase 1 (param registry) — slider widgets read from `inputs` structs.
VR UI primitives from Phase 2 slider panel.
