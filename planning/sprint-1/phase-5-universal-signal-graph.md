# Phase 5: Universal Signal Graph

## Goal

A runtime node graph that routes every signal in the system — XR poses, audio buffers,
controller input, simulation textures, draw calls — through a single typed dataflow fabric.
The graph replaces the current hard-wired component connections. POST `/graph` with a JSON
description and the scene changes live.

## Core concepts

### Everything is a source or a sink

| Source node      | Outputs                                      |
|------------------|----------------------------------------------|
| `head_pose`      | `XrPosef` (left eye, right eye, HMD center)  |
| `left_controller`| `XrPosef`, `bool` buttons, `float` axes      |
| `right_controller`| same                                        |
| `left_mic`       | `audio_buffer`                               |
| `right_mic`      | `audio_buffer`                               |

| Sink node        | Inputs                                       |
|------------------|----------------------------------------------|
| `renderer`       | `XrPosef` (left eye, right eye), `draw_call[]`|
| `audio_output`   | `audio_buffer`                               |

The renderer's eye-pose inputs are just ports. Rerouting a controller pose to them is
the mechanism for "holding your eyeballs in your fist".

### Push/pull evaluation model

Nodes do not declare a rate. Rate is determined by which sink is pulling:

- **`audio_output`** pulls its upstream recursively inside the audio callback at ~48kHz.
  Every node reachable from `audio_output` through `audio_buffer` edges runs at audio
  rate — not because it declared itself as such, but because it is on the pull path.
- **`renderer`** pulls its upstream inside the XR frame loop at ~90Hz. Every node
  reachable through frame-rate port types (`XrPosef`, `draw_call`, `texture`, etc.)
  runs at frame rate.
- **Push sources** (`head_pose`, controllers, button events) propagate forward when they
  fire, independently of any sink pulling.

Rate conversion requires an explicit node. Connecting an `audio_buffer` output to a
`float` input is a type error at connection time.

### Graph data structure

```cpp
struct NodeInstance {
    const EyeballsNodeDescriptor* desc;
    void*                         data;   // desc->create() result
    std::string                   id;     // unique in graph
};

struct Edge {
    std::string from_node, from_port;
    std::string to_node,   to_port;
};

struct Graph {
    std::vector<NodeInstance> nodes;
    std::vector<Edge>         edges;
};
```

Port values are passed between nodes via a typed value store keyed by `(node_id, port_name)`.
Type safety is enforced at edge-connection time (connect-time type check, runtime assert).

## JSON graph format

```json
{
  "nodes": [
    { "id": "sky",   "type": "sky_dome",      "params": { "sun_elevation": 0.3 } },
    { "id": "water", "type": "water_surface",  "params": { "wave_height": 5.0  } }
  ],
  "edges": [
    { "from": "sky.sun_dir",   "to": "water.sun_dir"   },
    { "from": "head_pose.hmd", "to": "renderer.left_eye" }
  ]
}
```

POST to `/graph` deserializes this, instantiates nodes via the component registry
(type name → `EyeballsNodeDescriptor`), and replaces the current graph atomically
(swap on the next frame boundary).

## LLM workflow

```
User speaks: "add a water surface with choppy waves"
    → Phase 3 transcription → text
    → Claude API with system prompt describing available node types + port schema
    → generates JSON graph diff or full replacement
    → POST /graph
    → scene updates
```

The companion's system prompt for Claude is auto-generated from the component registry
(all registered type names + their `inputs` schemas).

## Key Design Decisions

**Atomic graph swap**: the new graph is fully constructed before replacing the current one.
No partial-update in-place. This avoids frame-boundary races and keeps the swap path simple.

**Port values are copied, not referenced**: nodes do not hold pointers into each other's
data. The value store copies on write. This is safe for frame-rate data; audio-rate
data uses a dedicated lock-free path.

**Source nodes are not plugins**: `head_pose`, `left_controller`, `left_mic` etc. are
built-in nodes compiled into the app. They have access to the OpenXR session and AAudio
stream. Their `process()` reads from the XR/audio APIs and writes to their output ports.
They satisfy the plugin ABI so they appear in the graph uniformly.

## Dependencies

Phase 4 (plugin ABI) — all nodes are loaded through the descriptor.
Phase 2 (networking) — `/graph` HTTP endpoint.
Phase 1 (param registry) — `deserialize` populates `inputs` structs.
