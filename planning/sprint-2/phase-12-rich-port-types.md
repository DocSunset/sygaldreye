# Phase 12: Rich Port Types

## Goal

Expand the port type system beyond `slider<>` (scalar float) so every signal that flows
in the app — Eigen geometry, colors, GPU textures, audio buffers, draw calls — can be
declared as a typed port, wired in the graph, and propagated by `tick_graph`.

This phase must be completed before Phase 10's visual-node output and ABI work begins,
so those nodes use the right types from the start rather than accumulating a second
round of churn.

## Problem

Currently:
- Input ports: `slider<>` (float with range), `toggle<>` (bool), `bang<>` (event).
  Nothing for vectors, matrices, quaternions, colors, textures, or audio.
- Output ports: `texture_output<>` (GPU texture, push-only via `push_textures`).
  Nothing for scalars, vectors, matrices, draw calls declared in the same uniform way.
- ABI exchange: `set_input(void*, const char*, double)` — only doubles. No way to
  inject a `Vector3f` or `Matrix4f` along an edge.
- Value store: `std::unordered_map<std::string, double>` — scalars only.

With this as the foundation, every wired connection in the system would be forced
through float scalars, and compound types (sun direction as a `Vector3f`, MVP as a
`Matrix4f`, audio buffer as `float*` + frame count) would need to be decomposed and
recomposed by hand.

## Design

### `port<Name, T>` — generic typed port template

```cpp
// sygaldry_endpoints.hpp addition:
template<fixed_string Name, typename T>
struct port {
    static consteval std::string_view name() { return Name; }
    using value_type = T;
    T value{};
};
```

Used for both input and output ports where the field carries no slider UI metadata.
`slider<>` remains for scalar inputs that need range/init for the VR editor.

Usage:
```cpp
struct SkyDome {
    struct inputs {
        slider<"sun_elevation", "", float, fp(-0.5f), fp(1.f), fp(0.3f)> sun_elevation;
        port<"tint", Eigen::Vector4f> tint;   // no slider needed; wired from elsewhere
    } inputs;
    struct outputs {
        port<"sun_dir",    Eigen::Vector3f>  sun_dir;
        port<"sun_color",  Eigen::Vector4f>  sun_color;
        port<"render",     DrawFn>           render;    // see below
    } outputs;
};
```

### Supported port value types

| C++ type | Semantic | Notes |
|----------|----------|-------|
| `float` / `double` | scalar | `slider<>` covers inputs; `port<N,float>` for outputs |
| `bool` | flag | `toggle<>` covers inputs |
| `Eigen::Vector2f` | 2D coord, UV | |
| `Eigen::Vector3f` | position, direction, RGB | |
| `Eigen::Vector4f` | homogeneous, RGBA | |
| `Eigen::Matrix4f` | transform, MVP | |
| `Eigen::Quaternionf` | rotation | |
| `GpuTexture` | GPU texture handle | lightweight struct, not a copy of pixels |
| `AudioBuffer` | audio (new struct) | pointer + frame count, borrowed during tick |
| `DrawFn` | renderable callback | alias for `std::function<void(const Eigen::Matrix4f&)>` |

`AudioBuffer` (new, in `sygaldry_endpoints.hpp` or a separate header):
```cpp
struct AudioBuffer {
    const float* data    = nullptr;  // owned by the producing node
    int          frames  = 0;
    int          channels = 1;
    int          sample_rate = 48000;
};
```

`DrawFn` (alias, not a template — one per draw port):
```cpp
using DrawFn = std::function<void(const Eigen::Matrix4f& vp)>;
```

Visual nodes set `outputs.render.value = [this](const auto& vp){ draw(vp); }` in
their `operator()`.

### Typed value store

Replace `Graph::values` (`map<string,double>`) with a variant-based store:

```cpp
// signal_graph.hpp:
using PortValue = std::variant<
    double,
    Eigen::Vector2f,
    Eigen::Vector3f,
    Eigen::Vector4f,
    Eigen::Matrix4f,
    Eigen::Quaternionf,
    GpuTexture,
    AudioBuffer     // pointer valid only during tick_graph
>;

struct Graph {
    std::vector<NodeInstance>  nodes;
    std::vector<Edge>          edges;
    std::unordered_map<std::string, PortValue>  values;    // "node.port" → typed value
    std::vector<DrawFn>                          draw_calls; // collected each tick
    ~Graph();
};
```

`graph.textures` (the old separate texture map) is retired; textures now live in
`graph.values` as `GpuTexture` variants.

### ABI v4: typed setters and `push_outputs`

#### Typed input setters (replacing `set_input(void*, const char*, double)`)

Each ABI v4 setter field is nullable (old nodes that only have scalar inputs need
only `set_scalar_in`):

```c
void (*set_scalar_in) (void* node, const char* port, double v);
void (*set_vec2_in)   (void* node, const char* port, float x, float y);
void (*set_vec3_in)   (void* node, const char* port, float x, float y, float z);
void (*set_vec4_in)   (void* node, const char* port, float x, float y, float z, float w);
void (*set_mat4_in)   (void* node, const char* port, const float* col16);
void (*set_quat_in)   (void* node, const char* port, float x, float y, float z, float w);
void (*set_texture_in)(void* node, const char* port,
                       unsigned int gl_id, int w, int h,
                       unsigned int fmt, unsigned int filter);
void (*set_audio_in)  (void* node, const char* port,
                       const float* samples, int frames, int channels, int rate);
```

#### `push_outputs` with typed context

```c
typedef struct {
    void* store;           /* opaque: Graph's PortValue map */
    const char* node_id;
    void (*emit_scalar) (void*, const char* nid, const char* port, double v);
    void (*emit_vec2)   (void*, const char* nid, const char* port, float x, float y);
    void (*emit_vec3)   (void*, const char* nid, const char* port, float x, float y, float z);
    void (*emit_vec4)   (void*, const char* nid, const char* port, float x, float y, float z, float w);
    void (*emit_mat4)   (void*, const char* nid, const char* port, const float* col16);
    void (*emit_quat)   (void*, const char* nid, const char* port, float x, float y, float z, float w);
    void (*emit_texture)(void*, const char* nid, const char* port,
                         unsigned int gl_id, int w, int h, unsigned int fmt, unsigned int filter);
    void (*emit_audio)  (void*, const char* nid, const char* port,
                         const float* samples, int frames, int channels, int rate);
    /* draw_call outputs handled separately by push_draw_calls */
} EyeballsOutputCtx;
```

`push_textures` (v2) is superseded by `push_outputs` but remains in the struct for
ABI backwards compatibility with v1–v3 plugins.

#### Updated `EyeballsNodeDescriptor` (v4)

```c
#define EYEBALLS_ABI_VERSION 4

typedef struct {
    int         version;
    const char* type_name;
    const char* description;
    void*       (*create)(void);
    void        (*destroy)(void*);
    void        (*process)(void*, double);
    const char* (*serialize)(void*);
    void        (*free_str)(const char*);
    void        (*deserialize)(void*, const char*);
    void        (*push_textures)(void*, void*);    /* v2: legacy, keep for ABI compat */
    const char* source_header;                     /* v3 */
    const char* source_cpp;                        /* v3 */
    const char* port_schema;                       /* v4: static JSON, nullable */
    void        (*push_outputs)(void*, EyeballsOutputCtx*); /* v4 */
    void        (*push_draw_calls)(void*, void*);  /* v4: DrawCallCtx* in C++ */
    void        (*set_scalar_in) (void*, const char*, double);
    void        (*set_vec2_in)   (void*, const char*, float, float);
    void        (*set_vec3_in)   (void*, const char*, float, float, float);
    void        (*set_vec4_in)   (void*, const char*, float, float, float, float);
    void        (*set_mat4_in)   (void*, const char*, const float*);
    void        (*set_quat_in)   (void*, const char*, float, float, float, float);
    void        (*set_texture_in)(void*, const char*, unsigned int, int, int,
                                  unsigned int, unsigned int);
    void        (*set_audio_in)  (void*, const char*, const float*, int, int, int);
} EyeballsNodeDescriptor;
```

All v4 fields are nullable; `component_registry.load_plugin` accepts `version >= 1`.

### `make_descriptor<T>()` generation

PFR iterates `node.outputs`; for each field:
- `port<N, float>` or `slider<...>` → calls `ctx->emit_scalar`
- `port<N, Eigen::Vector2f>` → calls `ctx->emit_vec2`
- `port<N, Eigen::Vector3f>` → calls `ctx->emit_vec3`
- `port<N, Eigen::Vector4f>` → calls `ctx->emit_vec4`
- `port<N, Eigen::Matrix4f>` → calls `ctx->emit_mat4`
- `port<N, Eigen::Quaternionf>` → calls `ctx->emit_quat`
- `port<N, GpuTexture>` → calls `ctx->emit_texture` (supersedes `TextureOutputField` path)
- `port<N, AudioBuffer>` → calls `ctx->emit_audio`
- `port<N, DrawFn>` → skip (handled by `push_draw_calls`)

PFR iterates `node.inputs`; for each field:
- `slider<...>` or `port<N, float>` → calls `desc->set_scalar_in` 
- `port<N, Eigen::Vector3f>` → calls `desc->set_vec3_in`
- `port<N, GpuTexture>` → calls `desc->set_texture_in`
- etc.
- `toggle<>` → calls `desc->set_scalar_in` with 0.0 / 1.0 (bool cast)

Concepts added to `eyeballs_node_abi.hpp`:
- `PortField<F>` — any type with `name()` and `value` member
- `DrawCallField<F>` — `PortField` where `value_type` is `DrawFn`
- `ScalarField<F>` — `PortField` where `value_type` is `float` or `double`
- `VecField<F, T>` — `PortField` where `value_type` is an Eigen vector type
- etc.

### `port_schema` JSON format

Generated at compile time; describes all ports with type info:

```json
{
  "inputs": [
    {"name":"wave_height","kind":"scalar"},
    {"name":"sun_dir","kind":"vec3"},
    {"name":"albedo","kind":"vec4"},
    {"name":"env_texture","kind":"texture"}
  ],
  "outputs": [
    {"name":"render","kind":"draw_call"},
    {"name":"sun_dir","kind":"vec3"},
    {"name":"concentration","kind":"texture"}
  ]
}
```

`kind` values: `"scalar"`, `"bool"`, `"vec2"`, `"vec3"`, `"vec4"`, `"mat4"`,
`"quat"`, `"texture"`, `"audio"`, `"draw_call"`.

Used by Phase 11's VR editor to lay out port handles and color-code wires.

### `tick_graph` value injection (Phase 10 uses this)

For each edge `(from_node.from_port → to_node.to_port)`:
1. Read `graph.values[from_node + "." + from_port]` → `PortValue` variant.
2. `std::visit` to get the concrete type.
3. Call the appropriate `set_*_in` function on the destination node's descriptor.

Draw call edges are not represented in `graph.edges` — draw calls accumulate into
`graph.draw_calls` via `push_draw_calls` and are consumed by the renderer implicitly.
The graph JSON can describe a `draw_call` connection logically, but it is resolved
automatically (all draw_calls feed the renderer).

## Port type taxonomy summary

| Category | Input endpoint | Output endpoint | Wire kind |
|----------|---------------|-----------------|-----------|
| Scalar float | `slider<>` | `port<N,float>` | `scalar` |
| Bool | `toggle<>` | `port<N,bool>` | `bool` |
| Event | `bang<>` | `port<N,bool>` (momentary) | `bang` |
| 2D coord | `port<N,Vector2f>` | `port<N,Vector2f>` | `vec2` |
| Direction/position | `port<N,Vector3f>` | `port<N,Vector3f>` | `vec3` |
| Color / homogeneous | `port<N,Vector4f>` | `port<N,Vector4f>` | `vec4` |
| Transform / MVP | `port<N,Matrix4f>` | `port<N,Matrix4f>` | `mat4` |
| Rotation | `port<N,Quaternionf>` | `port<N,Quaternionf>` | `quat` |
| GPU texture | `port<N,GpuTexture>` | `port<N,GpuTexture>` | `texture` |
| Audio | `port<N,AudioBuffer>` | `port<N,AudioBuffer>` | `audio` |
| Rendered geometry | (not an input — implicit from draw_call collection) | `port<N,DrawFn>` | `draw_call` |

## Work item sequence

1. **`port<Name,T>` template + `AudioBuffer` + `DrawFn`** — add to
   `sygaldry_endpoints.hpp`; add `DrawFn` alias; add `AudioBuffer` struct.
2. **Typed value store** — change `Graph::values` from `map<string,double>` to
   `map<string,PortValue>` (variant); retire `graph.textures` (merge into `values`);
   add `graph.draw_calls`; update `parse_graph`, `serialize_graph`, `tick_graph`
   stubs (edge propagation not yet implemented — that's Phase 10).
3. **ABI v4 C header** — add `EyeballsOutputCtx`, typed setters, `push_outputs`,
   `push_draw_calls`, `port_schema` to `eyeballs_node_abi.h`; bump version to 4.
4. **`make_descriptor<T>()` update** — generate `push_outputs` (PFR over outputs,
   type-dispatch to `emit_*`), typed `set_*_in` functions (PFR over inputs),
   `push_draw_calls` (PFR over outputs, `DrawCallField` concept), `port_schema`
   (constexpr JSON built from PFR names + type tags); retire old
   `push_textures_fn` generation path.
5. **Update existing nodes to use `port<>`** — replace `texture_output<>` with
   `port<N, GpuTexture>` in `rd_gpu` and any other nodes that have outputs;
   update `make_descriptor` call sites; verify `sh/build.sh` clean.

Each work item: implement → unit test → `sh/build.sh` → commit.

## Dependencies

- `sygaldry_endpoints.hpp` (Phase 1 deliverable — extended here)
- `eyeballs_node_abi.h` / `.hpp` (Phase 4 deliverable — versioned here)
- Eigen (already in flake.nix)
- No new external dependencies

## What this enables

- Phase 10 visual nodes can declare `port<"render", DrawFn>` and `port<"sun_dir", Vector3f>` outputs.
- Phase 10 `tick_graph` can propagate any `PortValue` along an edge without knowing the type.
- Phase 11 VR editor reads `port_schema` to know how many handles to draw and what colors to use.
- Future: a `mat4` edge carries a full MVP matrix; a `texture` edge passes a GPU texture handle;
  an `audio` edge carries a sample buffer — all in the same graph, same routing fabric.
