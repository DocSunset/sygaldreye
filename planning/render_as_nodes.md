# Render as graph nodes: GL boundary + shader-specific nodes

Plan for the render-path half of task #59. Ratified direction (Travis,
2026-06-15). A shared C++ helper, and then even a `DrawDesc` payload, are
both the wrong target — they leave nodes *saying GL*, or re-encode wiring
as data. Do for rendering what #58 did for audio: pull device-facing work
into ONE boundary, and express the composition in graph NODES.

## The precedent (#58 audio)

Nodes stopped writing the device buffer. They produce planar
`AudioBuffer`s; `audio_region::render_block` is the ONLY place that
de-planarizes to the device. There is no `AudioDeviceDesc` payload — the
`dac` is a graph endpoint node and the boundary READS its resolved input.

## The target (rendering)

- No node calls `glDraw*`, compiles a program, or owns a VBO. The
  **renderer becomes the GL "device"** — a `render_region` boundary that
  owns all GPU resources and is the only place that says GL.
- The **draw node is the render endpoint** (the `dac` of rendering). It
  takes a `Mesh` and a `Material` (plus sequencing — below). After tick,
  `render_region` reads the resolved inputs of the draw endpoints and
  issues GL. The "draw description" is the node + its wiring (graph
  state) — never a struct on an edge. **There is no `DrawDesc` payload.**
- **Instancing is not a strategy.** A shader node's per-instance
  attribute input is a `Span`: 1 row → one instance; N rows → the
  boundary issues `glDraw*Instanced`. "Excess rank → lift" lands here.

## Payloads (things one node produces for another to consume)

Two new edge payloads, both emitted by shader-specific nodes:

- `Surface` — appearance: a `ShaderData` handle (vert+frag GLSL + stable
  id; the boundary compiles once and caches `GlProgram` by id) + bound
  uniform values + pipeline state (blend, depth test/write, cull). NOT a
  `GlProgram` — GL stays in the boundary. Named `Surface`, not
  `Material`: the existing `Material` struct is Blinn-Phong appearance
  params, an INPUT to the lit-shader node — kept as-is (Travis,
  2026-06-15).
- `Mesh` — drawable geometry: base geometry (`MeshPtr`/`TriMeshData`) +
  named per-instance attribute `Span`s (divisor 1; rows = instance
  count) + a GL-free `Primitive` enum (Triangles/Strip/Lines/Points).

Correct-by-construction: the shader-specific node knows its uniform set
and attribute layout, so no bind-by-name and no dynamic ports. Raw
geometry generators (cube/sphere/quad/points) emit plain `MeshPtr`, which
a shader node wraps; geometry stays reusable across shaders.

`MeshPtr`, `Span`, `GpuTexture`, scalars/vecs already exist. New:
`Surface`, `Mesh`. Removed: `DrawFn` (end of phase D).

## Existing infrastructure to reuse (surveyed 2026-06-15)

The render path is NOT a blank slate — graph-wrap what is already here:
- `GlProgram` — the boundary's program-cache primitive.
- `GlGeometry` — generic VAO + arbitrary `AttribDesc` layout +
  draw_arrays/elements. The boundary's geometry+instance VBO cache, built
  from a `MeshPtr`. Needs a per-attrib divisor + an instanced draw added.
- `LitShader`/`FlatShader`/`RgbaShader`/`cube_shader`/`wind_shader` —
  shader-specific wrappers (own a program, cache uniform locs, typed
  setters). The shader-specific NODES are the graph form of these; their
  GL/program ownership moves into the boundary.
- `Material` (Blinn-Phong params) + `Light` — INPUTS to shader nodes.
- `render_nodes` (`RenderTargetNode`/`TextureViewNode`) — existing v6
  offscreen FBO→texture nodes on the `DrawFn` edge; migrate in phase D.
- `host_gl_context` + `scene_snapshot` + `app_snapshot` — the headless
  EGL + PNG/MP4 verification path.

## Node set (full granularity)

- **shader-specific nodes** — one node type per shader. Static input
  ports = that shader's uniforms (→ `Material`) and its per-instance
  attributes + a geometry input (→ `Mesh`). Stamped per shader, the way
  kernels are stamped per ugen.
- **geometry nodes** — existing mesh generators emit `MeshPtr`; add
  `quad` and `points` primitives.
- **draw node** — generic. In: `Mesh`, `Material`, `event_in`. Out:
  `event_out`. On tick it enqueues itself (a handle, read at issue time)
  with `render_region`. Sequencing is orthogonal to appearance, so ONE
  draw-node type serves every shader (the `dac` of rendering).
- **render-head node** — emits a frame-start event and clears the
  boundary's ordered list. Root of the draw-order chain.

## Draw-call sequencing (the framebuffer is NOT commutative)

Audio's mix bus is commutative (sum); the framebuffer is not — blending
and depth-disabled overlays are order-dependent. So order must be
declarable. Thread it through events: head → draw → draw → … via
`event_out → event_in`. In the Frame region, appliers run BEFORE
`process()` in topological order, so an event reaches a downstream node
in the SAME tick; a linear chain is a DAG in one region (no z⁻¹, no
crossing), so topo order IS submission order. Each draw node enqueues
itself when it fires; `render_region` replays that order per eye.

Bonus, parallel to `dac`: a draw node NOT wired into the head's chain
never fires, so it is not drawn. "Only what reaches the head renders,"
in declared order. Sequence is the wiring.

## The boundary (`render_region`)

Mirrors `audio_region`. Owns: mesh-VBO cache keyed by `MeshPtr::get()`
(the identity nodes already use as `uploaded_`), program cache keyed by
`Shader` id, texture/instance streaming buffers. Per frame, replays the
ordered draw endpoints: bind program → apply pipeline state → set the
`Material` uniforms by name → set well-known render-time uniforms
unconditionally (`uProj`/`uView`/`uCameraRight`/`uCameraUp`/`uTime`;
absent → location -1 → silent no-op, so no per-draw "needs" list) → bind
the cached geometry VAO → upload+bind each per-instance `Span` at
divisor 1 → `glDrawElements` or `glDrawElementsInstanced(N)`. Lifetime is
safe: render runs same-thread, same-frame, right after tick.

## Phases (each step builds + is screenshot-verifiable headless on host)

### A — Foundations (no behavior change)
A1. Define `Shader`, `Material`, `Mesh` payloads; add to `PortValue`.
A2. `render_region` boundary: GPU caches + ordered replay + instancing.
A3. render-head node + event-sequenced draw node.
A4. **Bridge**: keep `DrawFn` `draw_calls` AND the new ordered endpoint
    list; `app.cpp` runs legacy closures, then `render_region` replays.
    Both coexist so nodes migrate one at a time (bridge deleted in D2).

### B — The nodes
B1. quad + points geometry. B2. the draw node + render-head (from A3).
B3. shader-specific nodes for the shaders the current renderers use
    (phong/lit mesh, billboard, points, glyph), each emitting
    `Material` + `Mesh`.

### C — Decompose the five hand-rollers (decomposition acceptance)
Re-express each as geometry → shader-node → draw + wiring; delete the
hand-rolled GL; screenshot-verify identical.
- C1. mesh_render → single draw (proves the path).
- C2. mesh_instances → mesh + scatter(Span) → draw; instancing falls
      out; delete the per-row loop and the node. (Forest route 2.)
- C3. particle_system → quad + sim-produced (pos,color,size) Span; the
      pool sim stays a node, only the DRAW leaves.
- C4. billboard_quad → quad + (pos,size) Span, camera-facing shader.
- C5. star_field → points + N positions → draw(Points).
- C6. text_mesh → glyph-layout node (text → per-glyph quad+uv Span) +
      glyph shader → draw(instanced).

### D — Migrate the rest, delete the bridge
D1. Remaining `DrawFn` producers (sky_dome, water_surface, terrain,
    reaction_diffusion, chladni, wire_mesh, scene) move over in waves.
D2. Delete `DrawFn`; `push_draw_calls` enqueues a draw endpoint instead
    of a closure (bump ABI version); remove the bridge. glDraw now lives
    ONLY in `render_region`.
D3. Move `TriMesh`/`GlProgram` ownership fully into `render_region`
    caches; delete node-owned GPU members.

### E — General lift (rung 3, non-GPU) + acceptance
E1. Executor excess-rank check in `tick_graph` for NON-draw inputs:
    stateless cell-map → loop; stateful map axis → N `clone_graph`
    instances (key field / index) via the migration machinery;
    resource-holder lift → error. (GPU instancing already falls out of C.)
E2. Forest two routes: route 2 done in C2; route 1 (N seeds → lifted
    tree_generator → N distinct meshes) needs a minimal mesh-array
    payload — add it here, not deferred tensor work.
E3. Update conformability.md; close #59.

## Decisions locked
- No `DrawDesc`, no generic material node, no bind-by-name: shader-
  specific nodes give static ports and correct-by-construction bundles.
- Payload is `Surface` (not `Material`); existing `Material` (Blinn-Phong)
  stays a shader-node input.
- Draw node is generic (`Mesh` + `Surface` + event ports); appearance is
  upstream, sequencing is orthogonal.
- Draw order is declared by the event chain from the render-head.
- GL lives only in `render_region`; reuse `GlProgram` + `GlGeometry`.
- Nodes only produce data; verification is headless via
  `host_gl_context` + `scene_snapshot`.

## Progress (2026-06-15, branch `render-as-nodes`)

- DONE A1: `Surface`/`Mesh` payloads in the variant (+ v6_kind, read_output).
- DONE A2: `render_region` boundary + `draw`/`render_head`/`color_mesh`
  nodes; host_app begin_frame/issue bridge. Cube verified.
- DONE instancing: per-instance Spans at divisor 1; `color_mesh`
  rank-polymorphic. Forest (80) + grove (multi-draw, 3 materials) verified.
- DONE Quest parity: `app.cpp` on the render path; `libeyeballs.so`
  cross-compiles (arm64-v8a).
- DONE sprites: render_region camera basis (uCameraRight/Up) + `sprite`
  node (camera-facing quad, alpha blend, instanced). 160 verified.

### DrawFn migration leg (2026-06-15, synthetic-sprouting-lantern plan)
- DONE R0 render_region capabilities: uTime, uViewPos, GpuTexture +
  CPU-image (ImageTex) uniforms, cull_front, additive blend, dynamic
  geometry (persistent slots + update_verts when topology stable).
- DONE R1 mesh producers: `vertex_color_mesh` node; terrain→geometry gen;
  sky_dome, water_surface, wire_mesh, lissajous (LineStrip), cube migrated.
- DONE R2 instanced: particle_system (per-instance pos/size/color),
  star_field (Points) migrated. billboard_quad is a helper, not a node.
- DONE R3 effects: chladni, reaction_diffusion (CPU sims → dynamic
  vertex-color), aurora_curtain (additive). visual_node deleted.
- DONE R4 text: CPU-image atlas path; `glyph::` layer; text_label MSDF.
- Deleted along the way: app_snapshot, mesh_render, mesh_instances,
  visual_node, particle_system_shader (legacy/redundant).
- DONE R5 editor rendering: editor decomposed into nodes (card_mesh,
  card_widgets_mesh, card_labels_mesh, editor_wires, palette_mesh +
  gesture nodes); monolith + poke_button/poke_stick/vr_panel/ray_selector
  DELETED. ui_nodes + control_panel.json deleted (no consumers).
- DONE R6 ordering: render_head→draw chain in editor.json (and scenes).
- DONE DrawFn teardown: `DrawFn` + the bridge DELETED, ABI bumped to v8.
  Only `scene_snapshot`'s unrelated SnapshotDrawFn remains.
- PARKED R7 offscreen: render_target/texture_view/rd_renderer/glsl_effect
  out of build (sources kept); revived on render_region FBO passes.
- TODO E: non-GPU rung-3 executor (subgraph → N clone_graph; CPU cell-map).
