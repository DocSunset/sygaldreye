# Node-ify the XR subsystem — retire the xr_sources pump

The `pump_xr_sources` model (app.cpp: a per-frame loop that type-dispatches
on node type and calls `set_state`/`set_pose` to shove XR data into specific
source nodes) is an anachronism. It's bespoke shell behavior the graph
depends on; sources should be self-contained nodes, like every other node.

Direction (Travis, 2026-06-16):
- **Controllers (and head) become Quest-only source nodes with outputs and
  NO dependency on shell pump behavior.** Their `operator()` tick "just so
  happens" to reach into xr-land to refresh their outputs — same shape as a
  `noise` or `time` node, just reading a different source of truth.
- **One node for both controllers is fine** (e.g. a single `controllers`
  node, or two instances of one parameterized node). Not a hard constraint.
- **Delete `pump_xr_sources` entirely** once nodes self-read.
- Eventually the same for **destinations** (haptics/rumble out), not just
  sources — xr is a two-way device.

## How a node reaches "xr-land" (design note)
Mirror the existing device-boundary pattern: `render_region` is the GL
"device" singleton the platform feeds each frame; `audio_region` is the
audio device. Add an **`xr_input` boundary** (singleton or platform-owned
context) that the OpenXR frame loop updates once per frame (xrLocateSpace /
xrSyncActions / xrGetActionState / located controller + head poses). The
controller/head source nodes read THAT in their tick. This keeps the
"node tick touches the device" rule without per-node-type shell dispatch —
the shell updates one boundary, nodes self-serve. (`HeadPoseNode`/
`ControllerNode`'s `set_state` API and the app.cpp dispatch both go away.)

## Host/Quest share the editor as a SUBGRAPH (Travis's correction)
It is NOT required (and was a wrong assumption) that host and device boot
the same `editor.json`. The editor is shared as a **subgraph**; each shell
has a thin boot graph that instantiates it and wires HARDWARE-SPECIFIC
sources around it:
- `editor.json` declares **inlets** for hand input (left/right pose +
  trigger/grip/thumb) instead of embedding `hand` nodes.
- `quest_editor.json` (~10 lines): instantiate `editor` (subgraph) +
  `controllers` (XR source node) + wire controller outputs → editor inlets.
  Eventually also editor destinations → haptics.
- `host_editor.json`: instantiate `editor` + whatever host input source
  exists (HTTP-driven `hand`, or a future mouse→hand node) → editor inlets.
This dissolves the "controllers aren't registered on host" non-issue: the
host boot graph simply doesn't reference Quest-only nodes.

## Interim already shipped (this is the thing this item replaces)
`pump_xr_sources` was extended to also feed the `hand` nodes inside
`editor.json` (hand_l←left, hand_r←right) so the in-headset editor has live
input NOW (app.cpp, the `type == "hand"` branch). That is the cheap wiring;
this item is the proper decomposition. Do it after the UI is nicer.

## Files (when undertaken)
- `components/xr_sources/*` (controller/head nodes self-read; drop set_state)
- new `components/xr_input/*` (the boundary the frame loop publishes to)
- `components/app/app.cpp` (delete `pump_xr_sources`; frame loop updates the
  xr_input boundary instead; xr.cpp for the OpenXR action/space plumbing)
- `assets/graphs/editor.json` (→ inlets for hand input, drop internal `hand`
  nodes), new `assets/graphs/quest_editor.json` + host boot graph
- `components/peer_core/*` + both shells (boot the per-shell wrapper graph)

## Dependencies / sequencing
- Independent of the lifting work; needs subgraph inlets (done) + the boot-
  graph-per-shell change. Defer until the editor UI is in better shape.
