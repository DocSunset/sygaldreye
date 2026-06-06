# Current State (as of 2026-06-06)

This document records the gap between phases 1–9 aspirations and what was actually
delivered. It is the ground-truth starting point for phase 10 planning.

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
- **`vr_editor`**: node palette, ray selector, panel rendering.
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

## What the app does today

Boots, renders (hard-wired), runs audio (hard-wired), serves HTTP, exposes 19 node
types via `/palette`. Posting a JSON graph instantiates nodes and stores the graph
object, but nodes do not drive rendering, audio, or each other. The graph is a
registry of objects with tweakable params, not a routing fabric.
