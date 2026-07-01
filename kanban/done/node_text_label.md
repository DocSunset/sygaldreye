# Node: `text_label`

## Goal

Replace the hardcoded `state.scene_.labels()` rendering loop in `app.cpp` (lines 434–436)
with a `text_label` graph node. Each node renders one MSDF text string at a
configurable world-space position/scale using the existing `TextMesh` infrastructure.

After this node is implemented, the labels loop and `state.text_mesh_` can be removed
from the render callback (the VR editor also uses `text_mesh_` — check this before
removing; if vr_editor depends on it, leave text_mesh_ in AppState and only remove the
labels loop).

## Feasibility Assessment

**Straightforward with one resource-sharing concern.**

`TextMesh::init()` loads a font atlas texture and creates a VAO/VBO. If each text_label
node owns its own `TextMesh`, each node allocates a separate copy of the 512×512 atlas
texture (~1 MB). With a handful of labels this is acceptable; for many labels it wastes
VRAM.

**Resolution for now:** each `TextLabelNode` owns a `TextMesh` by value with lazy `init()`.
A future optimisation can share the texture via a static `GpuTexture` singleton or a shared
`TextMesh*` injected from the app. Document the per-node cost; it does not block correctness.

`text` is a serialised string parameter, not a typed port (strings are not in `PortValue`).
It is serialised via a custom `to_json`/`from_json` that emits `{"text":"...","pos_x":...}`.
This requires overriding the default `serialize`/`deserialize` rather than relying on the
pfr-based `param_registry` helpers (which only handle slider/toggle fields).

Custom serialise/deserialise is already done by some nodes (those that need it). It is
boilerplate-heavy but not difficult: write a small JSON snippet manually.

## Component

`components/text_label/`

```
text_label/
  text_label.hpp
  text_label.cpp
  CMakeLists.txt
```

## Design

```cpp
class TextLabelNode {
public:
    static consteval std::string_view name() { return "text_label"; }

    struct inputs {
        slider<"pos_x",  "", float, fp(-50.f), fp(50.f), fp(0.f)>    pos_x;
        slider<"pos_y",  "", float, fp(-50.f), fp(50.f), fp(1.5f)>   pos_y;
        slider<"pos_z",  "", float, fp(-50.f), fp(50.f), fp(-2.f)>   pos_z;
        slider<"scale",  "", float, fp(0.01f), fp(5.f),  fp(0.3f)>   scale;
    } inputs;

    struct outputs {
        port<"render", DrawFn> render;
    } outputs;

    // Extra param not expressible as slider: handled in custom serialize/deserialize
    std::string text = "label";

    void operator()(double time_s);

    // Custom serialization (not auto-generated) to include 'text' string
    std::string to_json() const;
    bool from_json(std::string_view json);

private:
    TextMesh mesh_;
    bool     initialized_ = false;
};
```

Because `text` is not a `slider`, the node cannot use the auto-generated
`make_descriptor` serialize/deserialize. Instead, register it with explicit
serialize/deserialize lambdas that call `node->to_json()` / `node->from_json()`.

`operator()` builds a transform: `T(pos) * scale(s)` as an Eigen 4×4, then:
```cpp
outputs.render.value = [this, mvp_prefix = transform_](const Eigen::Matrix4f& pv) {
    mesh_.draw(text, pv * mvp_prefix);
};
```

`ensure_init()` defers `mesh_.init()` to first process() call (requires GL context).

## Descriptor Registration

```cpp
// In app.cpp (not using make_descriptor<> since serialize/deserialize are custom):
auto* desc = new EyeballsNodeDescriptor{};
desc->version   = EYEBALLS_NODE_ABI_VERSION;
desc->type_name = "text_label";
desc->create    = []() -> void* { return new TextLabelNode(); };
desc->destroy   = [](void* p) { delete static_cast<TextLabelNode*>(p); };
desc->serialize = [](void* p) -> const char* {
    auto s = static_cast<TextLabelNode*>(p)->to_json();
    char* out = static_cast<char*>(std::malloc(s.size() + 1));
    std::memcpy(out, s.data(), s.size() + 1);
    return out;
};
desc->free_str  = [](const char* s) { std::free(const_cast<char*>(s)); };
desc->deserialize = [](void* p, const char* json) {
    static_cast<TextLabelNode*>(p)->from_json(json);
};
// process, push_outputs, push_draw_calls, set_scalar_in: use pfr helpers or manual
state.registry_.register_builtin(desc);
```

Actually: it is cleaner to use `make_descriptor<TextLabelNode>()` for the automatically
generated lifecycle/ports and override only serialize/deserialize afterwards by patching
the returned descriptor pointer — or better, add `serialize()` / `deserialize()` as
detected methods in `eyeballs_node_abi.hpp` (like `HasProcess`). Check if such a concept
already exists. If not, implement the descriptor manually as shown above.

## App.cpp Cleanup

Remove:
- The `state.text_mesh_.draw(lbl.text, pv * lbl.transform)` loop (lines ~434–436)
- `TextMesh text_mesh_` from AppState **only if** `vr_editor_.draw` does not use it
  (check `vr_editor.hpp`; if it does, keep `text_mesh_` for the VR editor and just
  remove the labels loop)

## Acceptance Criteria

- `text_label` node renders configurable text at world-space position
- `text` param round-trips through JSON serialize/deserialize
- Labels loop removed from render callback
- `sh/build.sh` passes
