# Component Consolidation Study

_Date: 2026-06-06_

## Scope

Survey of all visual-node and infrastructure components looking for duplicated logic,
candidate sub-component extractions, and opportunities to express existing nodes as
subgraphs composed of smaller, reusable parts.

---

## Component Inventory

### Visual Nodes

| Component | What it does | GPU geometry style |
|-----------|-------------|-------------------|
| `aurora` | Animated translucent curtains, Blinn-Phong sway | Per-curtain VAO/VBO/EBO (custom) |
| `lissajous` | 3-D parametric curve, HSV rainbow | Single VAO/VBO (custom) |
| `chladni` | Chladni plate mode morphing, particle field | **TriMesh** |
| `reaction_diffusion` | Gray-Scott on triangle grid, ping-pong FBO | **TriMesh** + `rd_gpu` |
| `water_surface` | Gerstner-wave ocean, Blinn-Phong foam | Custom VAO/VBO/EBO (dynamic) |
| `sky_dome` | Procedural sky, sun disc, stars | Custom VAO/VBO (static) |
| `terrain_generator` | Perlin-noise terrain, 5 color bands, Blinn-Phong | `TriMesh` + `LitShader` |
| `particle_system` | Point-sprite emitter, gravity, instancing | Custom VBO (dynamic) |

### Infrastructure / Shared Utilities

| Component | Role |
|-----------|------|
| `gl_program` | Shader compile / link / uniform helpers |
| `tri_mesh` | Generic CPU triangle mesh (pos + normal + color) |
| `sphere_geometry` / `sphere_mesh` | Parametric sphere vertex data |
| `lit_shader` | Reusable Blinn-Phong shader pass |
| `gpu_texture` | Texture handle RAII |
| `light` / `material` | Shared POD structs for lighting |
| `rd_gpu` | Ping-pong FBO compute pass (Gray-Scott) |
| `sygaldry_endpoints` | `slider<>`, `port<>`, `DrawFn` port types |
| `eyeballs_node_abi` | Boost.PFR descriptor generation; runtime type info |
| `signal_graph` | Graph store + `tick_graph()` evaluation |
| `synth_core` | `Phasor`, `LFO`, `BiquadFilter` building blocks |

### XR / App

| Component | Role |
|-----------|------|
| `xr_sources` | `HeadPoseNode`, `LeftControllerNode`, `RightControllerNode` |
| `renderer_node` | Eye-pose inputs as graph node |
| `app` | Android entry, XR pump, graph tick, renderer dispatch |
| `renderer` | Eye swapchain, per-eye `on_draw` callback |
| `frame_loop` | OpenXR predict/wait/begin/end loop |

---

## Identified Duplications

### 1. VAO/VBO Setup — Major Duplication

The same four-step pattern appears in **6 of 8 visual nodes**, replicated from scratch:

```cpp
glGenVertexArrays(1, &vao_);
glGenBuffers(1, &vbo_);
glBindVertexArray(vao_);
glBindBuffer(GL_ARRAY_BUFFER, vbo_);
glBufferData(GL_ARRAY_BUFFER, n_bytes, data, GL_DYNAMIC_DRAW);
glEnableVertexAttribArray(0);
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
```

Affected files:

| File | Lines | Variation |
|------|-------|-----------|
| `aurora/aurora.cpp` | 164–177 | Per-curtain loop; EBO shared |
| `lissajous/lissajous.cpp` | 55–68 | Single VAO; 3 attribs (pos, normal, color) |
| `water_surface/water_surface.cpp` | 170–210 | Dynamic VBO; separate EBO |
| `particle_system/particle_system.cpp` | 220–240 | Instancing VBO alongside base VBO |
| `sky_dome/sky_dome.cpp` | 18–38 | Two VAOs (dome + star field) |
| `vr_panel`, `text_mesh`, `vr_editor_handles` | various | Same idiom in UI layer |

`chladni` and `reaction_diffusion` are the **only** visual nodes that correctly delegate
to `TriMesh::create()`. The remaining six carry ~50 lines of boilerplate each.

**Extraction candidate:** `GlGeometry` — a lightweight RAII wrapper around one
VAO + 1–2 VBOs + optional EBO, with an `AttributeLayout` descriptor describing
strides and offsets. `TriMesh` already solves this for the standard vertex format;
`GlGeometry` would handle arbitrary layouts.

---

### 2. `operator()` / Draw Callback — Structural Duplication

Every visual node implements the identical three-step lifecycle:

```cpp
void operator()(double time_s) {
    // 1. copy input slider values into internal params
    params_.foo = inputs.foo.value;
    // 2. advance simulation
    update(static_cast<float>(time_s));
    // 3. register draw callback
    outputs.render.value = [this](const Eigen::Matrix4f& vp) { draw(vp); };
}
```

This appears verbatim (with different field names) in:
`aurora.cpp:186–194`, `lissajous.cpp:107–116`, `chladni.cpp`, `reaction_diffusion.cpp`,
`water_surface.cpp`, `sky_dome.cpp:44–62`, `terrain_generator.cpp`, `particle_system.cpp`.

**Extraction candidate:** A CRTP `VisualNode<Derived>` base that provides
`operator()(double)` and requires `Derived::sync_params()`, `Derived::update(float)`,
`Derived::draw(const Eigen::Matrix4f&)`. This is zero-overhead (no vtable) and collapses
8 identical operator() bodies to one.

---

### 3. Blinn-Phong Lighting — Duplicated GLSL

`water_surface` and `terrain_generator` both contain inline GLSL implementing
Blinn-Phong with a `Light` / `Material` uniform block. The fragment shader code
is ~40 lines and substantively identical:

```glsl
vec3 N = normalize(vNormal);
vec3 L = normalize(uLightDir);
float diff = max(dot(N, L), 0.0);
vec3 V = normalize(uViewPos - vWorldPos);
vec3 H = normalize(L + V);
float spec = pow(max(dot(N, H), 0.0), uShininess);
fragColor = vec4(uAmbient + diff*uDiffuse + spec*uSpecular, 1.0);
```

`terrain_generator` already routes through `LitShader`; `water_surface` does not.
`sky_dome` and `lissajous` have custom shaders with no lighting. `aurora` and
`chladni` use additive / threshold coloring.

**Extraction candidate:** Migrate `water_surface` to use `LitShader` + a
custom wave-displacement vertex stage, separating geometry from shading.

---

### 4. `xr_sources` — Triple Duplication of Input-Node Boilerplate

`xr_sources.hpp` defines three structurally identical nodes:

```cpp
struct HeadPoseNode {
    struct outputs { port<"pos_x", float> pos_x; /* ... 7 fields */ };
    void set_state(XrPosef const&);
    void operator()(double) {}
};
struct LeftControllerNode { /* identical pattern, 11 fields */ };
struct RightControllerNode { /* identical pattern, 11 fields */ };
```

The three nodes share no implementation; the state-set logic is copy-pasted.

**Extraction candidate:** `PoseSourceNode<Tag>` template parameterised on a
tag type, or a `DataSourceNode<Outputs>` CRTP base that provides the
`operator()` no-op and `set_state` plumbing once.

---

### 5. Audio Synth Nodes — Structural Duplication

Seven synth nodes (`water_synth`, `fire_synth`, `atmos_synth`, `engine_synth`,
`creature_synth`, `chime_synth`, `rain_synth`) all subclass a `MonoSynth` base
and compose from `synth_core` building blocks (`Phasor`, `LFO`, `BiquadFilter`).

This pattern is **intentional and mostly sound** — `synth_core` already plays
the role of the reusable sub-component. However, each synth duplicates its own
`operator()` → fill-buffer → push-output lifecycle in the same way visual nodes
do. A `AudioNode<Derived>` CRTP analogous to `VisualNode<Derived>` would
centralize this.

---

## Subgraph Composition Opportunities

The `signal_graph` already supports multi-node graphs with typed edges. The
following existing nodes could be expressed as subgraphs of smaller primitives:

### 5.1 `sky_dome` → `SkyShader` + `StarField` + `SunDisc`

`sky_dome` currently mixes three conceptually independent rendering concerns
inside one monolithic node:

- **Sky gradient** — a full-screen dome with altitude-based color ramp
- **Sun disc** — a bright billboard at the sun's computed direction
- **Star field** — randomised point sprites, visible only at night

These are independent draw passes that happen to share `sun_elevation` as
a parameter. A subgraph would have:

```
[SkyParams]──► [SkyShaderNode]──► render
           ──► [SunDiscNode]  ──► render
           ──► [StarFieldNode]──► render
```

In this form, a user could swap in a different sky model while keeping the
same star field, or attach the sun direction output to `water_surface` for
consistent lighting — currently impossible without modifying `sky_dome`'s
internals.

**sky_dome already exposes `sun_dir` and `sun_color` as output ports** — the
decomposition is well-motivated and partially prefigured.

---

### 5.2 `water_surface` → `GerstnerWavesMesh` + `OceanShader`

`water_surface` bundles two conceptually separable stages:

- **Geometry stage** — CPU Gerstner summation, vertex upload to VBO
- **Shading stage** — Blinn-Phong with foam threshold, sun direction

Splitting these mirrors `terrain_generator`'s existing `TriMesh` + `LitShader`
decomposition:

```
[GerstnerWavesMesh] ──mesh──► [OceanShader] ──► render
[SkyDome]          ──sun_dir──►
```

The mesh node would expose `cell_size`, wave frequency/amplitude as inputs
and emit a `DrawableMesh` port. The shader node would receive the mesh and
lighting parameters. This makes ocean + sky lighting consistent automatically
via the graph edge rather than manual uniform duplication.

---

### 5.3 `reaction_diffusion` → `RDSimulation` + `RDRenderer`

`reaction_diffusion` already separates GPU compute (`rd_gpu`) from rendering
(`reaction_diffusion.cpp`), but this boundary is hidden inside the monolithic node.

Exposing it as a two-node subgraph:

```
[RDSimulation] ──texture──► [RDRenderer] ──► render
```

allows the texture to be consumed by _other_ shaders (e.g., use the RD pattern
as a foam mask on `water_surface`, or as a displacement map on `terrain_generator`).
The current architecture makes this re-use impossible without forking.

---

### 5.4 XR Data Sources → Unified `XRInputNode` Subgraph

```
[XRSession] ──raw_poses──► [HeadPoseNode]        ──► pos/rot outputs
                       ──► [LeftControllerNode]   ──► pos/rot/trigger outputs
                       ──► [RightControllerNode]  ──► pos/rot/trigger outputs
```

Currently these are three separate manually-updated C++ objects that bypass the
graph; wiring them as first-class graph nodes with edges would allow downstream
nodes to react to controller position the same way they react to any other
parameter — including the planned HTTP/OSC control plane.

---

### 5.5 Audio + Visual Coupling (New Opportunity)

No current graph wires audio synth outputs to visual node inputs. The architecture
supports it — `signal_graph` is type-agnostic. Example subgraphs that would be
expressible with zero new C++ code:

```
[WaterSynth] ──rms──► [WaterSurface foam_threshold]
[AtmosSynth] ──lfo──► [SkyDome sun_elevation]
[Aurora]     ──ripple_amplitude──► driven by microphone rms via [MicCapture]
```

The primary missing piece is an **adapter node** that converts `float` audio
samples / envelope values to the float ranges expected by visual sliders.

---

## Priority Ranking

| Priority | Item | Effort | Impact |
|----------|------|--------|--------|
| 1 | **`VisualNode<Derived>` CRTP base** | Small (1–2 hours) | Eliminates 8× operator() duplication; enforces contract |
| 2 | **Migrate `water_surface` to `LitShader`** | Medium (half day) | Removes duplicate GLSL; unlocks consistent sun lighting |
| 3 | **`sky_dome` → 3-node subgraph** | Medium (half day) | Makes `sun_dir` a first-class graph edge; enables cross-node lighting |
| 4 | **`reaction_diffusion` → `RDSimulation` + `RDRenderer`** | Small | Unlocks texture reuse across nodes |
| 5 | **`GlGeometry` utility** | Medium | Removes ~300 lines of boilerplate across 6 nodes |
| 6 | **`DataSourceNode<>` template for xr_sources** | Small | Collapses 3× identical input-node boilerplate |
| 7 | **Audio ↔ visual adapter node** | Small | Enables cross-domain routing with no new per-node code |

---

## What Would Not Benefit from Consolidation

- **Per-node shader GLSL** — shaders are semantically unique; premature unification
  leads to complex `#ifdef` tangles. Each node's vertex/fragment shader should
  remain inline to that node.
- **`synth_core` building blocks** — already well-factored; no further extraction needed.
- **`GlProgram`** — already a clean shared utility; no further work needed.
- **`TriMesh`** — already correct for `chladni` and `reaction_diffusion`; the
  question is adoption by the remaining nodes, not further factoring.

---

## Conclusion

The architecture's ABI and graph primitives are sound. The primary technical
debt is **geometry boilerplate** (VAO/VBO setup) and **operator() boilerplate**
(input-sync → update → render-callback), each replicated 6–8 times. Both are
small, focused extractions.

The larger architectural opportunity is expressing composite nodes as subgraphs:
`sky_dome` decomposition and the `reaction_diffusion` texture-port are the
highest-leverage items because they unlock cross-node data flow that is currently
structurally impossible. The audio ↔ visual adapter is essentially free once the
graph edge typing is solid.
