# Phase 11: VR Editor Completion

## Goal

A fully self-contained in-VR editing loop. While the app is running, the user can:
browse the node palette, create nodes, observe the current graph, drag ports to connect
them, tweak scalar parameters by grabbing sliders, disconnect wires, and delete nodes —
without removing the headset or touching a keyboard.

The canonical validation: wire `right_controller.pos_*` / `rot_*` → `renderer.left_eye_*`
and `renderer.right_eye_*` in VR, then move your hand and watch the viewpoint follow.

## Prerequisite: renderer as a graph node (Phase 10 work item 8b)

The renderer must appear in the graph as a node so the VR editor can show its input
ports as connectable handles.

```cpp
struct RendererNode {
    static consteval std::string_view name() { return "renderer"; }
    struct inputs {
        // Each eye: 7 optional override scalars. If wired, overrides XR session values.
        slider<"left_pos_x",  "", float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_x;
        slider<"left_pos_y",  "", float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_y;
        slider<"left_pos_z",  "", float, fp(-5.f), fp(5.f), fp(0.f)> left_pos_z;
        slider<"left_rot_x",  "", float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_x;
        slider<"left_rot_y",  "", float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_y;
        slider<"left_rot_z",  "", float, fp(-1.f), fp(1.f), fp(0.f)> left_rot_z;
        slider<"left_rot_w",  "", float, fp(-1.f), fp(1.f), fp(1.f)> left_rot_w;
        // right_pos_*/right_rot_*: same
    } inputs;
    struct outputs {} outputs;
    void operator()(double) {}
    bool has_left_override()  const;  // true if any left_* input is wired
    bool has_right_override() const;
};
```

The app frame loop reads `RendererNode::inputs` after `tick_graph` to override
`xrLocateViews` pose for each eye. Inputs that are at their init value AND not wired
are treated as "no override". A dedicated flag per-eye (`left_eye_wired_`,
`right_eye_wired_`) is set during `parse_graph` when edges point to these inputs.

The default startup graph always includes a `renderer` node.

## Port handles

Each node card gains visible port handles: small colored discs positioned to the left
of the card (inputs) and right (outputs). One handle per port in `inputs` /
`outputs` structs, spaced vertically.

```cpp
struct PortHandle {
    std::string  port_name;
    bool         is_output;
    Eigen::Vector3f world_pos;
    // type tag for color coding: scalar, draw_call, texture
    enum class Kind { Scalar, DrawCall, Texture } kind;
};
```

Handle positions are recomputed in `VrEditor::on_graph_changed` from the card position
and port count. Handles are rendered as small quads (or spheres via `sphere_geometry`)
using `RgbaShader`, color-coded by kind.

Port metadata comes from `EyeballsNodeDescriptor`. We need to expose it:

```c
/* v4 addition to ABI (alongside serialize_outputs / set_input / push_draw_calls) */
const char* (*port_schema)(void); /* Returns static JSON: {"inputs":[{...}],"outputs":[{...}]}
                                     Each port: {"name":"...", "kind":"scalar|draw_call|texture"} */
```

`make_descriptor<T>()` generates this at compile time via PFR over `inputs` and
`outputs`. The string is `constexpr`-generated and returned as a static literal (no
malloc needed).

`VrEditor` calls `port_schema()` once per node type and caches the result to lay out
handles.

## Parameter sliders on node cards

For each scalar input port, the node card shows an inline horizontal slider widget.

```cpp
struct SliderWidget {
    Eigen::Vector3f world_pos;
    float width, height;
    std::string node_id, port_name;
    float value, min_val, max_val;

    // Returns updated value if the controller intersects and trigger is held.
    std::optional<float> update(const XrPosef& controller, bool trigger);
    void draw(const Eigen::Matrix4f& vp, RgbaShader& shader) const;
};
```

Controller intersection with a slider: project controller position onto the slider's
plane, compute UV, map to [min, max]. While trigger is held and controller is within
the slider bounds, call `desc->set_input(node.data, port_name, value)` and also POST
`/params` (or update the pending graph JSON) so the change persists on the next graph
swap.

The node card height scales with input port count.

## Wire rendering

Wires are rendered as line strips (or thin quads) between connected port handle
positions. For each edge `(from_node.from_port → to_node.to_port)`:

1. Look up the output handle position of `from_node.from_port` in the card map.
2. Look up the input handle position of `to_node.to_port`.
3. Render a cubic bezier with control points biased outward (so wires curve naturally
   away from cards).

Wires are drawn after all cards and handles, using `RgbaShader` with the same
kind-based color coding as handles.

## Drag-to-connect

State machine in `VrEditor`:

```
IDLE → [grip on output handle] → DRAGGING(from_node, from_port, from_pos)
DRAGGING → [release over compatible input handle] → emit GraphEdit(new edge)
DRAGGING → [release over nothing] → IDLE (no change)
DRAGGING → [release over incompatible handle] → IDLE (haptic error pulse)
```

While `DRAGGING`:
- Render a "ghost wire" bezier from `from_pos` to the current controller tip position.
- Highlight nearby input handles: green if kinds match, red if not.

Type checking: two ports are compatible if they have the same `Kind`. In practice:
`scalar` connects to `scalar`; `draw_call` cannot be connected manually (it is
implicit in the render path). `texture` connects to `texture`.

On successful connect:
1. Add `Edge{from_node, from_port, to_node, to_port}` to the current graph.
2. Re-serialize the graph (including edges this time — Phase 10 fixes the round-trip).
3. Return `GraphEdit{new_json}` from `update()`.

`app.cpp` receives `GraphEdit`, passes JSON to the existing atomic graph-swap path.

## Delete

Point the right controller at a node card or wire for 1 second → delete with a haptic
pulse.

- **Delete node**: remove the node from the graph and all edges referencing it.
- **Delete edge**: find the edge closest to the controller ray, remove it.

Both produce a `GraphEdit` with the new JSON. The previous graph JSON (from
`serialize_graph` before the change) is stored as a single-level undo buffer.

Undo: dedicated thumbstick-left gesture → POST previous JSON.

## Layout and interaction summary

```
Left hand (non-dominant):
  - Palette panel floats near left palm (world-locked to hand, or fixed nearby)
  - Tap palette row with right controller → spawn node card in front of user

Right hand (dominant):
  - Ray extends from controller
  - Tap slider track while holding trigger → drag slider value
  - Grip output port handle → start dragging wire
  - Release wire over input handle → connect
  - Point at card/wire for 1s → delete
  - Thumbstick left → undo last graph change
```

## VrEditor API changes

```cpp
struct VrEditor {
    void init(const ComponentRegistry&, const Graph*);
    void on_graph_changed(const Graph*);

    struct GraphEdit { std::string new_graph_json; };

    // Called each frame. Returns non-nullopt if the graph should change.
    std::optional<GraphEdit> update(
        const XrPosef* left_pose, const XrPosef* right_pose,
        bool trigger_left,        bool trigger_right,
        bool grip_right,          // new: for wire dragging
        const Eigen::Vector2f& thumbstick_left,  // new: for undo gesture
        const Graph* current_graph,
        const ComponentRegistry& registry);

    void draw(const Eigen::Matrix4f& vp, const TextMesh& text) const;

private:
    // existing
    RgbaShader           shader_;
    VrPanel              palette_panel_;
    std::vector<VrPanel> node_cards_;
    std::vector<std::string> palette_types_;
    std::vector<std::string> card_ids_;
    bool prev_trigger_right_ = false;

    // new
    std::vector<std::vector<PortHandle>> input_handles_;   // [card_idx][port_idx]
    std::vector<std::vector<PortHandle>> output_handles_;
    std::vector<std::vector<SliderWidget>> sliders_;       // [card_idx][scalar_port_idx]
    std::vector<Edge> current_edges_;
    std::string undo_json_;

    // drag state
    enum class DragState { Idle, Dragging } drag_state_ = DragState::Idle;
    std::string drag_from_node_, drag_from_port_;
    Eigen::Vector3f drag_from_pos_{};
    bool prev_grip_right_ = false;

    // delete state
    float point_dwell_s_ = 0.f;
    int pointed_card_idx_ = -1;
    int pointed_edge_idx_ = -1;
};
```

## Port schema in the ABI

The `port_schema()` function returns a static (not malloc'd) JSON string:

```json
{
  "inputs":  [{"name":"wave_height","kind":"scalar"}, ...],
  "outputs": [{"name":"render","kind":"draw_call"}, {"name":"sun_elevation_out","kind":"scalar"}]
}
```

`make_descriptor<T>()` builds this at compile time using a `constexpr` buffer
populated by PFR iteration over `inputs` and `outputs`. Because the schema is static
per type (not per instance), there is no malloc; `port_schema` is a raw `const char*`
field on the descriptor (not a function pointer), or a nullary function returning a
static string literal.

Decision: use a field `const char* port_schema;` (nullable). Simpler than a function
pointer since the value is statically known. This is an ABI v4 field.

## Work item sequence

1. **`RendererNode`** — built-in node with 14 scalar inputs (2 eyes × 7 pose
   components); app reads overrides after `tick_graph`; included in default startup
   graph. *(Belongs in Phase 10 WI-8b; document here as prerequisite.)*
2. **`port_schema` ABI field** — `const char* port_schema;` on descriptor; nullable;
   `make_descriptor<T>()` generates constexpr JSON from PFR over inputs+outputs;
   `EYEBALLS_ABI_VERSION` bumped to 4 (alongside other v4 additions from Phase 10).
3. **`PortHandle` struct + layout** — `VrEditor::on_graph_changed` builds handle
   positions from `port_schema`; handles rendered as colored quads.
4. **`SliderWidget`** — inline slider on each scalar input; controller intersection +
   trigger-hold sets value; returns delta for graph update.
5. **Wire rendering** — bezier lines for each edge in `current_edges_`.
6. **Drag-to-connect state machine** — grip output handle → ghost wire → release on
   input handle → `GraphEdit`.
7. **Delete + undo** — point-and-hold card/wire → remove; thumbstick-left → restore.
8. **Integration in `app.cpp`** — pass `grip_right` and `thumbstick_left` to
   `VrEditor::update`; handle returned `GraphEdit` through the existing graph-swap path.

## Dependencies

Phase 10 (all work items) must be complete: value propagation, draw_call collection,
renderer-as-node, XR sources live, edge round-trip in JSON. Without those, the VR
editor changes the graph object but nothing visible changes in the scene.

## Deferred

- Pie/marking menu for node spawn (replace palette tap)
- Speech-driven graph edits ("add a water surface") integrated with editor visual feedback
- Wire bundling and auto-layout for large graphs
- Multi-user simultaneous editing (Phase 6 material)
