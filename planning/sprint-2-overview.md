# Sprint 2 Overview (starting 2026-06-06)

## Primary user story

> While the app is running, in VR, without removing the headset or touching a keyboard,
> I can browse registered nodes, create instances, observe the current graph, drag ports
> to wire them together, tweak parameters by grabbing sliders, and disconnect wires.
> The canonical demo: wire a controller pose to the renderer's eye-pose inputs and
> hold my eyeballs in my hand.

## Sprint 2 phases

| Phase | Title | Enables |
|-------|-------|---------|
| 10 | Signal flow | Graph actually routes values; renderer driven by graph |
| 11 | VR editor completion | In-headset create, wire, tweak, delete |

## What phases 10 and 11 together must deliver

- POST `/graph` with a JSON graph → scene changes visually (Phase 10)
- Scalar values flow along edges (sky sun_elevation → water sun_elevation) (Phase 10)
- XR source nodes publish live pose data as scalar outputs (Phase 10)
- `renderer` is a proper graph node with wireable eye-pose inputs (Phase 10)
- Port handles visible on node cards in VR (Phase 11)
- Drag from output handle to input handle → edge added to graph (Phase 11)
- Inline parameter sliders on node cards (Phase 11)
- Wires rendered between connected port handles (Phase 11)
- Delete node or edge with point-and-hold (Phase 11)

This document records the gap between phases 1–9 aspirations and what was actually
delivered. It is the ground-truth starting point for sprint 2 planning.

## What exists

### Infrastructure (solid)

- **`sygaldry_endpoints`**: `slider<>`, `toggle<>`, `bang<>`, `texture_output<>`,
  `fixed_string`, `fp` (structural float wrapper for NDK Clang 18 NTTP workaround).
- **`param_registry`**: `to_json`/`from_json` over any `struct inputs` via Boost.PFR.
  `sscanf` fallback for float parsing (NDK libc++ omits `std::from_chars` for float).
- **`eyeballs_node_abi` v3**: C-linkage descriptor — `create`, `destroy`, `process`,
  `serialize`, `deserialize`, `free_str`, `push_textures` (v2),
  `source_header`/`source_cpp` (v3).
- **`make_descriptor<T>()`**: reflection-generated descriptor. `EYEBALLS_EXPORT_NODE`
  is the only manual boilerplate.
- **`component_registry`**: `register_builtin`, `load_plugin` (dlopen/RTLD_LOCAL),
  `find`, `type_names`, `~ComponentRegistry` (dlclose).
- **`http_server`**: Mongoose, GET/POST `/params`, GET/POST `/graph`, POST `/plugins`,
  GET `/palette`, GET `/events` (SSE), GET `/plugins`.
- **`mdns_advertiser`**: mdns.h single-header, Android JNI multicast lock.
- **`mic_capture`**, **`push_to_talk`**: AAudio mic, left-trigger rising/falling edge.
- **`vr_editor`**: palette panel listing all registered types; spawning a node by
  tapping palette row (builds and POSTs new graph JSON). Node cards drawn as colored
  quads with type name text. **Missing**: port handles, parameter sliders, wire
  rendering, drag-to-connect, delete.
- **`rd_gpu`**: Gray-Scott GPU simulation (Android only).
- **19 nodes registered**: all visual renderers, audio synths, XR sources have
  `name()`, `source_header/cpp()`, `struct inputs`, `operator()(double)`.

### Signal graph (shell only)

`signal_graph` has the right data structures and compiles, but the routing fabric
itself is not implemented:

```cpp
void tick_graph(Graph& g, double time_s) {
    for (auto& n : g.nodes) {
        if (n.desc->process) n.desc->process(n.data, time_s);
        if (n.desc->push_textures) { ... }  // textures only
    }
}
```

`serialize_graph` hardcodes `"edges":[]`. `parse_graph` ignores the `"edges"` array.

## Gap table

| Feature | Spec | Reality |
|---------|------|---------|
| Edge JSON round-trip | parsed and serialized | `parse_graph` ignores edges; `serialize_graph` emits `"edges":[]` |
| Value propagation | outputs flow along edges to inputs | `tick_graph` calls `process()` only; no propagation |
| Topological sort | nodes run in dependency order | insertion order |
| `outputs` structs | all nodes declare outputs | no node has `struct outputs` |
| `draw_call` port type | visual nodes emit draw calls | not implemented |
| Renderer as graph sink | renderer lives in graph | hard-wired in `app.cpp` |
| Audio as graph pull chain | `audio_output` pulls at audio rate | AudioScene wired directly in `app.cpp` |
| XR sources live | frame loop pumps pose data into source nodes | `HeadPoseNode::operator()` is a no-op; no frame-loop integration |
| `set_input` ABI | efficient per-port scalar injection | not in ABI |
| `serialize_outputs` ABI | nodes publish output port values | not in ABI |
| `push_draw_calls` ABI | nodes register draw call callbacks | not in ABI |
| Linux spectator renders graph | polls /graph, renders it | hard-wired WaterSurface+SkyDome |
| SSE on graph change | broadcast when POST /graph succeeds | param changes only |

## VR editor gap (detailed)

`vr_editor.cpp` (113 lines) currently:
- Draws palette panel with type labels; maps ray-hit UV → type index
- On right-trigger tap against palette: injects a new node into `serialize_graph()`
  output and returns a `GraphEdit` with the new JSON
- Draws node cards (one colored quad per node) with the node ID as a text label

It does **not**:
- Show input or output port handles (no visual ports on cards)
- Show slider widgets for scalar inputs (no in-VR parameter tweaking)
- Render wires between connected nodes (no `graph.edges` displayed)
- Support dragging from an output handle to an input handle (no connection creation)
- Support deleting nodes or edges
- Allow grabbing and repositioning node cards

Because there are no port handles and no wire rendering, there is currently **no way**
to create edges in VR. The only way to connect two nodes is to POST JSON from an
external client (the companion script or curl).

## What the app does today

Boots, renders (hard-wired), runs audio (hard-wired), serves HTTP, exposes 19 node
types via `/palette`. Posting a graph JSON instantiates nodes; tapping the palette in
VR can spawn new nodes. But nodes do not drive rendering, edges are not displayed or
manipulable in VR, and no parameters can be tweaked from inside the headset. The graph
is a registry of objects with tweakable params via HTTP only, not a routing fabric and
not an in-VR interactive system.
