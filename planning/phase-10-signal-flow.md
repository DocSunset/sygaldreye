# Phase 10: Signal Flow

## Goal

Make the graph a real routing fabric. After this phase:

- Edges round-trip through JSON (parse and serialize).
- `tick_graph` propagates scalar values along edges (outputs → value store → inputs).
- `tick_graph` runs nodes in topological order.
- `tick_graph` collects draw calls from visual nodes into `graph.draw_calls`.
- The renderer draws from `graph.draw_calls`; hard-wired scene members removed.
- XR source nodes receive live pose data from the frame loop each tick.
- Default startup graph is a JSON file, not hard-wired code.

Audio at true audio rate (pull chain from `audio_output`) is deferred to Phase 11.
Audio synths continue feeding `AudioScene` directly; `tick_graph` just keeps them
updated at frame rate so their input ports respond to edges.

## ABI v4

Three additions, all nullable (backwards-compatible with v1–v3 plugins):

```c
#define EYEBALLS_ABI_VERSION 4

/* v4 — all nullable: */

/* Returns malloc'd JSON object of output port values. Free with free_str.
   NULL if node has no scalar/bool outputs. */
const char* (*serialize_outputs)(void* node);

/* Sets a named scalar input port. No-op if port_name not found.
   NULL if node has no settable inputs. */
void (*set_input)(void* node, const char* port_name, double value);

/* If node has draw_call outputs, appends callbacks to DrawCallCtx.
   NULL if node has no draw_call outputs. */
void (*push_draw_calls)(void* node, void* draw_ctx);
```

`DrawCallCtx` (in `eyeballs_node_abi.hpp`):

```cpp
struct DrawCallCtx {
    std::string node_id;
    std::vector<std::function<void(const Eigen::Matrix4f& vp)>>* calls;
};
```

`make_descriptor<T>()` generates all three via concept detection + if constexpr:

- `HasOutputs` concept → `serialize_outputs`, `push_draw_calls`
- `HasScalarInputs` concept → `set_input` (PFR name lookup, set `.value`)
- `HasDrawCallOutputs` concept (outputs contains a `draw_call<>` field) → `push_draw_calls`

## `draw_call` port type

Add to `sygaldry_endpoints.hpp`:

```cpp
template<fixed_string Name>
struct draw_call {
    static consteval std::string_view name() { return Name; }
    std::function<void(const Eigen::Matrix4f& vp)> fn;
};
```

Not JSON-serializable (carries a function pointer). `serialize_outputs` skips
`draw_call` fields; `push_draw_calls` handles them exclusively.

## Output port declarations

Every node that produces values for downstream consumption adds `struct outputs`.

**Visual nodes** — all get a `draw_call` render output:

```cpp
struct WaterSurface {
    struct inputs { ... } inputs;
    struct outputs {
        draw_call<"render"> render;
    } outputs;
    void operator()(double t) {
        // existing sim update ...
        outputs.render.fn = [this](const Eigen::Matrix4f& mvp) { draw(mvp); };
    }
};
```

Same pattern: `SkyDome`, `Lissajous`, `Aurora`, `Chladni`, `TerrainRenderer`,
`ParticleSystem`, `ReactionDiffusion`, `RdGpu`.

**`SkyDome`** — also exposes scalar outputs for inter-node wiring:

```cpp
struct outputs {
    draw_call<"render"> render;
    slider<"sun_elevation_out", "", float, fp(-0.5f), fp(1.f),  fp(0.3f)> sun_elevation_out;
    slider<"sun_azimuth_out",   "", float, fp(0.f),   fp(6.28f),fp(0.785f)> sun_azimuth_out;
};
// operator() copies from inputs.sun_elevation.value → outputs.sun_elevation_out.value
```

**XR source nodes** — decompose pose into scalar outputs:

```cpp
struct HeadPoseNode {
    struct inputs {} inputs;
    struct outputs {
        slider<"pos_x",  "", float, fp(-5.f), fp(5.f), fp(0.f)> pos_x;
        slider<"pos_y",  "", float, fp(-5.f), fp(5.f), fp(0.f)> pos_y;
        slider<"pos_z",  "", float, fp(-5.f), fp(5.f), fp(0.f)> pos_z;
        slider<"rot_x",  "", float, fp(-1.f), fp(1.f), fp(0.f)> rot_x;
        slider<"rot_y",  "", float, fp(-1.f), fp(1.f), fp(0.f)> rot_y;
        slider<"rot_z",  "", float, fp(-1.f), fp(1.f), fp(0.f)> rot_z;
        slider<"rot_w",  "", float, fp(-1.f), fp(1.f), fp(1.f)> rot_w;
    } outputs;
    void set_pose(const XrPosef& p);  // writes to outputs.pos_x.value etc.
    void operator()(double) {}        // no-op; set_pose is called externally
};
```

`LeftControllerNode` and `RightControllerNode` add `trigger`, `grip`,
`thumbstick_x`, `thumbstick_y`.

## `tick_graph` algorithm

```
Graph::draw_calls.clear();

nodes_sorted = topological_sort(nodes, edges);  // DFS; log cycle if found

for each node in nodes_sorted:
    // Apply incoming edge values
    for each edge E where E.to_node == node.id:
        key = E.from_node + "." + E.from_port
        if key in graph.values and node.desc->set_input:
            node.desc->set_input(node.data, E.to_port.c_str(), graph.values[key])

    // Process
    if node.desc->process: node.desc->process(node.data, time_s)

    // Collect outputs
    if node.desc->serialize_outputs:
        json = node.desc->serialize_outputs(node.data)
        parse_flat_json(json) → graph.values[node.id + "." + key] = value
        node.desc->free_str(json)

    if node.desc->push_textures:
        GraphTextureCtx ctx{node.id, &graph.textures}
        node.desc->push_textures(node.data, &ctx)

    if node.desc->push_draw_calls:
        DrawCallCtx ctx{node.id, &graph.draw_calls}
        node.desc->push_draw_calls(node.data, &ctx)
```

`parse_flat_json` is a minimal helper: reads `{"key": value, ...}` and writes doubles
into `graph.values`. Skips non-numeric values (draw_call fields don't appear here).

`Graph` gains:
```cpp
std::vector<std::function<void(const Eigen::Matrix4f&)>> draw_calls;
```

## Edge round-trip in JSON

`parse_graph` parses the `"edges"` array:
```
{ "from": "sky.sun_elevation_out", "to": "water.sun_elevation" }
→ Edge{ from_node="sky", from_port="sun_elevation_out",
         to_node="water",  to_port="sun_elevation" }
```

`serialize_graph` emits actual `graph.edges` instead of hardcoded `[]`.

## `RendererNode` — renderer as a wireable graph node

The renderer must be a proper node in the graph so Phase 11's VR editor can display
and connect its eye-pose input ports.

```cpp
struct RendererNode {
    static consteval std::string_view name() { return "renderer"; }
    struct inputs {
        slider<"left_pos_x",  "", float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_x;
        slider<"left_pos_y",  "", float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_y;
        slider<"left_pos_z",  "", float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_z;
        slider<"left_rot_x",  "", float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_x;
        slider<"left_rot_y",  "", float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_y;
        slider<"left_rot_z",  "", float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_z;
        slider<"left_rot_w",  "", float, fp(-1.f), fp(1.f), fp(1.f)> left_rot_w;
        slider<"right_pos_x", "", float, fp(-5.f), fp(5.f), fp(0.f)> right_pos_x;
        slider<"right_pos_y", "", float, fp(-5.f), fp(5.f), fp(0.f)> right_pos_y;
        slider<"right_pos_z", "", float, fp(-5.f), fp(5.f), fp(0.f)> right_pos_z;
        slider<"right_rot_x", "", float, fp(-1.f), fp(1.f), fp(0.f)> right_rot_x;
        slider<"right_rot_y", "", float, fp(-1.f), fp(1.f), fp(0.f)> right_rot_y;
        slider<"right_rot_z", "", float, fp(-1.f), fp(1.f), fp(0.f)> right_rot_z;
        slider<"right_rot_w", "", float, fp(-1.f), fp(1.f), fp(1.f)> right_rot_w;
    } inputs;
    struct outputs {} outputs;
    void operator()(double) {}
};
```

The app frame loop reads `RendererNode::inputs` after `tick_graph`. If any left-eye
input has been touched by an edge (determined by whether any edge's `to_node` is the
renderer node and `to_port` starts with `left_`), it constructs an `XrPosef` override
for that eye and passes it to the render callback instead of the `xrLocateViews` value.

## Renderer draw call integration

In `app.cpp`'s per-eye render callback, replace all direct `->draw()` calls with:

```cpp
for (auto& call : active_graph_->draw_calls) call(eye_mvp);
```

Remove from `AppState`: `water_`, `sky_`, `scene_`, `cube_mesh_`, `text_mesh_`,
`particle_system_`, and any other hard-wired scene members. They live in the graph.

## XR source integration

In `app.cpp`'s frame loop, before `tick_graph`:

```cpp
for (auto& n : active_graph_->nodes) {
    auto* name = n.desc->type_name;
    if (std::string_view(name) == "head_pose")
        static_cast<HeadPoseNode*>(n.data)->set_pose(frame_hmd_pose);
    else if (std::string_view(name) == "left_controller")
        static_cast<LeftControllerNode*>(n.data)->set_state(left_state);
    else if (std::string_view(name) == "right_controller")
        static_cast<RightControllerNode*>(n.data)->set_state(right_state);
}
```

## Default startup graph

Move out of hard-wired code into `assets/default_graph.json`:

```json
{
  "nodes": [
    { "id": "sky",   "type": "sky_dome",     "params": { "sun_elevation": 0.3 } },
    { "id": "water", "type": "water_surface", "params": { "wave_height": 5.0  } },
    { "id": "head",  "type": "head_pose",     "params": {} }
  ],
  "edges": [
    { "from": "sky.sun_elevation_out", "to": "water.sun_elevation" }
  ]
}
```

App loads this at startup via the asset manager (Android) or filesystem (host).

## SSE on graph change

In the POST `/graph` handler, after successful atomic swap:

```cpp
http_server_.broadcast_event("graph", serialize_graph(*active_graph_));
```

(Small addition; included in this phase.)

## Nodes requiring `outputs` struct

| Node | Outputs |
|------|---------|
| `sky_dome` | `render` (draw_call), `sun_elevation_out`, `sun_azimuth_out` |
| `water_surface` | `render` (draw_call) |
| `lissajous` | `render` (draw_call) |
| `aurora` | `render` (draw_call) |
| `chladni` | `render` (draw_call) |
| `terrain_renderer` | `render` (draw_call) |
| `particle_system` | `render` (draw_call) |
| `reaction_diffusion` | `render` (draw_call) |
| `rd_gpu` | `render` (draw_call); `concentration` texture already via `push_textures` |
| `head_pose` | `pos_x/y/z`, `rot_x/y/z/w` |
| `left_controller` | same + `trigger`, `grip`, `thumbstick_x/y` |
| `right_controller` | same |

## Deferred to Phase 11

- True audio-rate pull chain (`audio_output` as graph sink pulling `audio_buffer` edges)
- Linux spectator rendering graph contents
- `XrPosef` as a compound port type (currently decomposed to 7 scalars)
- VR editor wire rendering (connecting ports visually)

## Work item sequence

1. **ABI v4** — add `serialize_outputs`, `set_input`, `push_draw_calls`, `port_schema`
   to the C header; update `make_descriptor<T>()` to generate them; bump
   `EYEBALLS_ABI_VERSION` to 4.
2. **`draw_call` port type** — add to `sygaldry_endpoints.hpp`; add `DrawCallCtx` and
   `HasDrawCallOutputs` to `eyeballs_node_abi.hpp`.
3. **Visual node outputs** — add `struct outputs { draw_call<"render"> render; }` and
   set `outputs.render.fn` in `operator()` for all 9 visual nodes.
4. **`SkyDome` scalar outputs** — add `sun_elevation_out`, `sun_azimuth_out`; copy from
   inputs in `operator()`.
5. **XR source outputs** — decompose `XrPosef` into scalar output ports; `set_pose`/
   `set_state` write them.
6. **`RendererNode`** — built-in node with 14 eye-pose override inputs; registered in
   component registry; app reads overrides after `tick_graph`.
7. **Edge round-trip** — fix `parse_graph` and `serialize_graph`.
8. **`tick_graph` overhaul** — topological sort + input application + output collection
   (scalars, textures, draw calls).
9. **App.cpp integration** — pump XR sources, draw from `graph.draw_calls`, remove
   hard-wired members, load default graph from file, apply `RendererNode` eye overrides.
10. **SSE on graph change**.

Each work item is a self-contained commit: implement → unit test → `sh/build.sh` → commit.
Items 1–2 are prerequisites for 3–5; item 6 is independent; item 7 depends on 1–5;
items 8–9 depend on 7.
