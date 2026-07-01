# Lifting core → structural editor decomposition → DrawFn retirement

Arc plan (2026-06-15, Travis + Claude). Sequenced, file-complete. Each
step lists **every** implicated file. Offscreen rendering is explicitly
deferred to the end (a separate, later leg). Ratified design behind this:
`kanban/backlog/conformability.md`, `kanban/ready/conformability_executor.md`,
`kanban/ready/vr_editor_decomposition.md`, `planning/conformability_lifting.md`,
`planning/subgraph.md`, `planning/render_as_nodes.md`.

## Order of operations (why this sequence)

1. **Part I — conformability lifting core (rung 3).** The executor learns
   ONE implicit-lifting rule; every port across audio/graphics/control is
   made lift-ready. Acceptance: forest route 1 + a lifted subgraph.
2. **Part II — structural editor decomposition.** Built ON Part I: the
   editor's cards are a *lifted card-subgraph over the live graph's nodes,
   keyed by id*. The editor's C++ ends up entirely inside small nodes; the
   `VrEditor`/`EditorNode` monolith is deleted. This is the acceptance
   test for subgraph lifting.
3. **Part III — DrawFn full retirement.** With the editor decomposed and
   the few remaining DrawFn producers migrated or parked, delete `DrawFn`
   and the whole bridge; bump the ABI.
4. **Deferred — offscreen.** `render_target`/`texture_view`/`rd_renderer`
   move onto `render_region` FBO passes. Parked (not deleted) during Part
   III so DrawFn can die; revived after.

Part II strictly depends on Part I. Part III depends on Part II (editor is
a DrawFn holdout) and on parking the offscreen nodes. Within each part the
steps are ordered by dependency.

---

# PART I — Conformability lifting core (rung 3)

## The one rule (from conformability.md)

"Match cells, lift frames, broadcast scalars, wrap like mc audio." A
port's **natural cell** is inferred from its kind (scalar→1, vec2→2,
vec3→3, vec4→4, quat→4, mat4→16; audio/span/mesh/surface/texture→whole
payload). The executor lifts on **excess rank only**: when a producer
emits an array whose *cell* matches a consumer's scalar/vector kind, the
edge lifts along the array's leading axis. The named axis (#58:
`Axis::Item/Cell/Channel/Time`), not the count, picks semantics:

- **map** (Item/Channel axis) → N independent **stateful** instances, one
  state each, identity by key-field (keyed) or index.
- **scan** (Time axis) → ONE instance threading state sample-to-sample
  (the audio `Lift` case; already compile-time-stamped per kernel).

Everything is liftable — kernels, builtins, subgraphs (= N `clone_graph`
clones) — **except resource-holders** (dac, editor, spawner, net, mic,
tts/stt, sample/wav players, render_target, rd_gpu). Lifting one, or a
subgraph containing one, is an error with a good message.

**DRY mandate:** ONE excess-rank check, ONE strategy table. No node
hand-rolls a lift. The four historic hand-lifters already converged on
per-instance Spans during the render-as-nodes leg (mesh_instances
deleted; color_mesh/sprite consume per-instance Spans directly) — this
part adds the *executor* that selects strategies, and the *runtime*
graph-scope cases (Span cell-map, subgraph N-clone).

## Design

### Natural-cell rank lives in the port schema
`make_descriptor` already emits each port's `kind`. Extend the derivation
so the schema reader can answer "what is one cell of this port" and "is
this port a whole-array consumer." The cell rank is a pure function of
kind, so it need NOT widen the ABI JSON for the common case — derive it in
`port_schema_reader`. The only thing that needs to travel is the rare
**whole-array opt-out** for a vector/scalar port that must NOT lift
(reduce/gather and buffer-as-unit consumers); audio/span/mesh ports are
whole-by-kind already, so most "whole" intent falls out for free.

### Lift detection at plan build
Today each edge becomes a literal `wire` (v6 pointer), an `applier`, a
`delay`, or a `crossing` in `build_plan`. Add a fifth resolution: when
`out_kind(producer)` is an array (`span`/`audio` with a Cell col-axis,
or a mesh-array payload) and the consumer port's kind is the array's cell
kind, record a **LiftGroup** instead of a wire. The check is one helper
beside `out_kind`.

### LiftGroup in the TickPlan
A new record the schedulers replay:
```
struct LiftGroup {
    std::size_t        host;        // consumer node index in g.nodes
    std::string        in_port;     // the lifted input
    EdgeApplier        source;      // producer Span/array
    port_types::Axis   axis;        // Item/Channel (map) vs Time (scan)
    enum { CellMap, Clones, Scan } strategy;
    std::vector<void*> instances;   // stateful: N node datas / clone_graphs
    // gather: lifted outputs concatenate to a plan-owned Span slot the
    // downstream consumer's src points at (mirrors audio_slots).
    Span               gather;
    std::vector<float> gather_buf;
};
```
Instance lifecycle reuses the **node-migration machinery**
(`migrate_graph` / serialize-swap-deserialize): as N changes between
ticks, instances are added/removed and matched by key (id) or index, so a
lifted instance keeps its state when the array reorders. Subgraph clones
use `clone_graph` (already exists) + the subgraph plan/wire path.

### Strategy table (implemented ONCE each)
- **CellMap** (stateless host): loop the host's `process` over each row
  with the lifted input bound to row r; concatenate outputs to `gather`.
  Requires a `lift_kind = stateless` declaration (safe-default is
  stateful).
- **Clones** (stateful host / subgraph): N instances, each ticked with
  its row; outputs gather. Builtins → N `create()`d datas; subgraphs → N
  `clone_graph`. Migrated across ticks.
- **Scan** (Time axis): the existing `ugen_detail::Lift` compile-time
  stamp. The generic runtime Scan (non-audio time axes) is rank>2 work —
  **deferred**; assert-unreached for now.
- **Resource-holder guard**: a `lift_kind = resource_holder` host (or a
  subgraph containing one) → hard error.
- **GPU instanced-draw (rung 5)**: an N×attribute Span into a renderable
  (`draw`/`color_mesh`/`sprite`) is ALREADY the instanced path. Document
  it as the renderer's lift strategy; the executor leaves these as wires
  (the boundary instances). No CPU loop.

### ABI / descriptor additions (bump to v7)
Add to `EyeballsNodeDescriptor`: `int lift_kind` (0 stateful default, 1
stateless, 2 resource_holder) and optional `const char* lift_key` (key
field for keyed identity). `make_descriptor` reads them from optional
static node members (`static constexpr LiftKind lift_kind()` etc.),
defaulting conservatively. Subgraph descriptors infer
`resource_holder` if any inner node is one.

## Steps (each: build + host gtest + Quest cross-compile + verify)

### L0 — schema & ABI plumbing for rank (no behavior change)
Derive cell rank from kind; add the whole-array opt-out and the
`lift_kind`/`lift_key` descriptor fields; bump ABI to 7. Nothing lifts
yet; every existing graph behaves identically.
Files:
- `components/eyeballs_node_abi/eyeballs_node_abi.h` (ABI v7; new fields)
- `components/eyeballs_node_abi/eyeballs_node_abi.hpp` (`make_descriptor`
  emits `lift_kind`/`lift_key`; `detail::v6_kind` stays the kind source)
- `components/eyeballs_node_abi/eyeballs_node_abi.test.cpp` (v7 fields)
- `components/port_schema_reader/port_schema_reader.hpp` (`PortInfo`
  gains `cell_rank` + `bool whole`)
- `components/port_schema_reader/port_schema_reader.cpp`
- `components/port_schema_reader/port_schema_reader.test.cpp`
- `components/sygaldry_endpoints/sygaldry_endpoints.hpp` (a `whole<T>`
  input wrapper / trait for the opt-out; `Axis` already present)
- `components/sygaldry_endpoints/sygaldry_endpoints.test.cpp`
- `components/signal_graph/signal_graph.test.cpp` (descriptor literals add
  the new fields — currently list every fn pointer)
- `components/subgraph_node/subgraph_node.test.cpp` (same descriptor lits)

### L1 — LiftGroup detection + the Clones strategy (the core)
Detect excess-rank edges in `build_plan`; build LiftGroups; tick them via
the Clones strategy (the safe default); gather outputs into plan-owned
Span slots; reset/point consumer `src` in `wire_plan`. Instance migration
reuses `migrate_graph`.
Files:
- `components/signal_graph/signal_graph_plan.hpp` (`LiftGroup`, plan
  vectors, `whole`/cell helpers next to `EdgeApplier`)
- `components/signal_graph/signal_graph_plan.cpp` (`build_plan`:
  excess-rank branch; `wire_plan`: gather-slot wiring + instance reset)
- `components/signal_graph/signal_graph_tick.cpp` (`tick_graph`: replay
  LiftGroups — instantiate/migrate, tick each, gather; shared
  `apply_value`/`read_output` unchanged)
- `components/signal_graph/signal_graph.hpp` (Graph holds lifted-instance
  storage if not plan-owned; `clone_graph` already declared)
- `components/signal_graph/signal_graph_serial.cpp` (`clone_graph` used
  for subgraph clones — verify owned_descriptors path for inline)
- `components/signal_graph/signal_graph_migrate.cpp` (factor the
  serialize-swap-deserialize into a reusable per-instance migrate)
- `components/signal_graph/signal_graph_sort.hpp` / `signal_graph_sort.cpp`
  (lifted host keeps ONE topo slot; gather is its output — confirm no
  reordering needed)
- `components/signal_graph/signal_graph.test.cpp` (new: excess-rank →
  N instances, outputs concatenate, state survives reorder)
- `components/subgraph_node/subgraph_node.hpp` / `.cpp` (subgraph as a
  liftable unit: clone + inner plan/wire per instance)
- `components/subgraph_node/subgraph_descriptor.cpp` (mark
  `resource_holder` if any inner node is)
- `components/subgraph_node/subgraph_node.test.cpp` (lift a subgraph → N
  clones, independent state)

### L2 — CellMap (stateless) strategy + resource-holder guard
Add the cheap stateless loop and the error path. Mark resource-holders.
Files:
- `components/signal_graph/signal_graph_tick.cpp` (CellMap loop)
- `components/signal_graph/signal_graph_plan.cpp` (strategy selection from
  `lift_kind`; guard → error)
- `components/dac_node/dac_node.hpp` (declare `resource_holder`)
- `components/spawner_node/spawner_node.hpp` (`resource_holder`)
- `components/editor_node/editor_node.hpp` (`resource_holder` — until
  Part II deletes it)
- `components/net_nodes/net_nodes.hpp` (udp send/recv `resource_holder`)
- `components/mic_input/mic_input.hpp`, `components/sample_player/…`,
  `components/wav_player/…`, `components/tts_node/…`,
  `components/tts_local/…`, `components/stt_whisper/…`,
  `components/whisper_node/…`, `components/speech_to_text/…`,
  `components/rd_gpu/rd_gpu.hpp`, `components/render_nodes/render_nodes.hpp`
  (each: `resource_holder`)
- `components/signal_graph/signal_graph.test.cpp` (guard error test;
  stateless CellMap test)

### L3 — "every port ready" sweep (audio + graphics + control)
Audit each input port across all 49 port-declaring files: confirm its
intended cell, add the `whole<T>` opt-out where a port consumes an array
as a unit (reduce/gather/mix/buffer-as-whole), and a `lift_key` where a
keyed identity is meaningful. Audio ports are whole-by-kind (Channel×Time
handled by `Lift`); most graphics/control ports are cells. Each file
either changes (opt-out/key/lift_kind) or is confirmed no-op in the doc.
Files (every endpoint-declaring node — audit all, change the exceptions):
- audio/control: `components/ugens/ugens.hpp` (mix/mc_pack take whole
  buffers — confirm), `components/math_nodes/math_nodes.hpp`,
  `components/osc_node/osc_node.hpp`, `components/spatialize_node/…`,
  `components/spectrogram/spectrogram.hpp` (FFT = whole time axis),
  `components/ptt_gate/ptt_gate.hpp`, `components/trigger_edge/…`,
  `components/sample_player/…`, `components/wav_player/…`,
  `components/sun_light/sun_light.hpp`, `components/fly_camera_node/…`,
  `components/hand_node/hand_node.hpp`, `components/xr_sources/…`,
  `components/mic_input/…`, `components/dac_node/…`,
  `components/claude_tmux/claude_tmux.hpp`, `components/net_nodes/…`,
  `components/tts_node/…`, `components/tts_local/…`,
  `components/stt_whisper/…`, `components/whisper_node/…`,
  `components/speech_to_text/…`
- graphics/mesh: `components/mesh_nodes/mesh_nodes.hpp` (ScatterNode &
  deformers — prime CellMap/lift candidates), `components/color_mesh/…`,
  `components/vertex_color_mesh/…`, `components/sprite/sprite.hpp`,
  `components/cube_node/cube_node.hpp`, `components/terrain_generator/…`,
  `components/sky_dome/sky_dome.hpp`, `components/water_surface/…`,
  `components/wire_mesh/wire_mesh.hpp`, `components/lissajous/…`,
  `components/chladni/chladni.hpp`, `components/reaction_diffusion/…`,
  `components/aurora_curtain/…`, `components/star_field/star_field.hpp`,
  `components/particle_system/…`, `components/text_label/text_label.hpp`,
  `components/sun_light/…`, `components/render_region/render_region_nodes.hpp`
  (draw/render_head), `components/renderer_node/renderer_node.hpp`,
  `components/glsl_effect/glsl_effect.hpp`, `components/rd_gpu/…`,
  `components/rd_renderer/…`, `components/render_nodes/…`
- ui/editor (also touched in Part II): `components/ui_nodes/ui_nodes.hpp`,
  `components/poke_button/poke_button.hpp`,
  `components/poke_stick/poke_stick.hpp`,
  `components/editor_node/editor_node.hpp`

### L4 — acceptance: forest route 1 + rung-5 doc
Prove the rule end-to-end. Route 1 (N seeds → lifted `tree_generator` →
N distinct meshes) needs a minimal **mesh-array** gather (N meshes out as
a Span-of-mesh-handles or a `vector<MeshPtr>` payload). Route 2
(instanced draw) already passes. Update conformability docs.
Files:
- `components/tree_generator/tree_generator.hpp` / `.cpp` /
  `tree_generator.test.cpp` (seed input liftable; emits one mesh)
- `components/render_payloads/render_payloads.hpp` (mesh-array gather
  payload if a `vector<MeshPtr>` is needed; or reuse Span semantics)
- `components/signal_graph/signal_graph.hpp` (`PortValue` arm if a
  mesh-array payload is added)
- `components/mesh_nodes/mesh_nodes.hpp` / `.cpp` (ScatterNode →
  confirm it feeds the lifted path, retire any residual manual loop)
- `components/integration_real_nodes/integration_real_nodes.test.cpp`
  (forest route 1 asserts N meshes; route 2 unchanged)
- `assets/graphs/` (a `forest.json` acceptance scene)
- `planning/conformability_lifting.md`, `kanban/ready/conformability_executor.md`,
  `kanban/backlog/conformability.md`, `planning/status.md` (mark rung 3 done)

---

# PART II — Structural editor decomposition into nodes

Goal: the editor's only C++ lives **inside small graph nodes**; the
`VrEditor` monolith (6 files) and `EditorNode` are deleted. Cards are a
**lifted card-subgraph over the live graph's nodes, keyed by id** — the
direct acceptance test for Part I's subgraph lifting. Follows the ratified
peel order in `kanban/ready/vr_editor_decomposition.md` (slice 1, wires,
already done).

## The meta seam: `graph_source` + `edit_sink`
The editor needs the WHOLE graph as data and emits edits against it.
Today: `PeerCore::pump_contexts` injects raw pointers via
`EditorNode::set_context`. Decomposed:
- **`graph_source`** — a resource-holder node the shell points at the live
  graph (replacing `set_context`). Outputs the graph model as data:
  `node_ids` (text array), `node_types` (text array), `card_positions`
  (N×3 Span), per-node port schemas, and `edges` (Span). This is the one
  place that reads the enclosing graph.
- **`edit_sink`** — consumes structured edit commands (text) and pushes
  them onto the same `edit_events_` queue `collect_edits` drains. Replaces
  the editor's whole-graph-JSON splicing with structured ops.

## Steps

### S1 — `graph_source` / `edit_sink` nodes (the meta seam)
Files:
- `components/graph_source/` (NEW: `graph_source.hpp/.cpp/.design.md/`
  `CMakeLists.txt/.test.cpp`) — reads live graph → data outputs; a
  `resource_holder`
- `components/edit_sink/` (NEW: same fileset) — text edit ops →
  `edit_events_`
- `components/peer_core/peer_core.hpp` / `peer_core.cpp` (replace the
  `type=="editor"`/`"spawner"` special-casing in `pump_contexts` with a
  generic "point graph_source/edit_sink at the live graph + edit queue";
  keep `on_graph_swapped`)
- `components/peer_core/peer_core.design.md`
- `components/host_app/host_app.cpp`, `components/app/app.cpp` (register
  the two new nodes)
- `CMakeLists.txt` (add the two subdirectories)
- `components/peer_core/peer_core.test.cpp` (if present) /
  `components/integration_http/…` (meta seam smoke test)

### S2 — structured edit ops (replace JSON splicing)
Move `remove_node` / `remove_edge_at` / add-node / add-edge / param-set
out of `vr_editor_interactions.cpp` string surgery into structured ops
emitted to `edit_sink` and applied in `parse_graph`/`collect_edits`.
Files:
- `components/edit_sink/edit_sink.*` (op vocabulary)
- `components/signal_graph/signal_graph_serial.cpp` (apply structured ops,
  or a new `signal_graph_edit.cpp` applying ops to a Graph/JSON)
- `components/peer_core/peer_core.cpp` (`collect_edits` accepts ops)
- `components/vr_editor/vr_editor_interactions.cpp` (delete the string
  surgery as ops replace it — file shrinks toward deletion)

### S3 — hit-test + gesture nodes (the interaction C++ → nodes)
Each gesture becomes a small stateful node consuming controller Spans +
the `graph_source` model, emitting edit ops. The drag FSM, slider, dwell
timer, undo, palette spawn, card-move move here.
Files (NEW node components, each hpp/cpp/design/CMake/test):
- `components/handle_picker/` — controller tip vs handle/slider/card Spans
  → nearest index + hover (from `vr_editor.cpp` hover loop +
  `vr_editor_interactions.cpp` nearest-handle)
- `components/wire_drag/` — grip-edge FSM → connect/move edit ops (from
  `update_drag`)
- `components/slider_drag/` — tip → value → param-set op (from
  `update_sliders` + `vr_editor_handles.cpp` slider layout)
- `components/dwell_delete/` — dwell timer → remove op (from `update_dwell`)
- `components/undo_node/` — thumb gesture → restore op (from `update_undo`)
- `components/palette/` — type list + poke → add-node op + paging (from
  `vr_editor.cpp` palette block)
- existing reusable nodes already proven: `poke_stick`, `poke_button`,
  `grab_detector`, `grab_target`, `ray_selector`, `text_label`,
  `tethered_point` — wire these in rather than re-implement
- `CMakeLists.txt`, `components/host_app/host_app.cpp`,
  `components/app/app.cpp` (subdirs + registrations)

### S4 — the card subgraph, lifted over `graph_source.nodes`
Author ONE card as a subgraph (`assets/graphs/card.json`): inlets
`{id, type, schema, position}`; builds the card panel + per-port handles +
sliders + the id label. Lift it over `graph_source`'s node array, **keyed
by id** (Part I's keyed Clones strategy) → N cards, each keeping drag
position across reorder. Palette rows = per-type lift; handles = per-port
lift inside the card. This is the rung-3 acceptance for subgraphs.
Files:
- `assets/graphs/card.json` (NEW — the card subgraph)
- `assets/graphs/editor.json` (NEW — graph_source → lifted card subgraph →
  draw chain + the gesture nodes from S3; replaces the embedded editor)
- `components/signal_graph/signal_graph_serial.cpp` (inline/keyed subgraph
  lift round-trips — verify with `card.json`)
- `components/subgraph_node/subgraph_node.*` (keyed lift identity)
- `components/component_registry/component_registry.cpp/.hpp` (load
  `card.json` as a subgraph type if by-reference)

### S5 — editor rendering as nodes (rides Part III's payloads)
Card/handle/slider geometry → vertex-color `Mesh`; id/label text → glyph
`Mesh` (reuse `text_label`/`glyph::` from the render leg); wires already
`wire_mesh`. No node says GL.
Files:
- the card subgraph (`assets/graphs/card.json`) wires `vertex_color_mesh`
  + `text_label` + `draw`
- `components/text_label/text_label.hpp/.cpp` (reuse; multi-string at
  transforms if needed)
- `components/vertex_color_mesh/vertex_color_mesh.hpp` (reuse)
- `components/wire_mesh/wire_mesh.*` (reuse)
- delete the bespoke quad/text path (see S6)

### S6 — delete the monolith
When `editor.json` reproduces the editor's behavior, delete the C++
monolith and its GL helpers (no remaining consumers).
Files DELETED:
- `components/editor_node/` (entire: hpp, cpp, design, CMake)
- `components/vr_editor/` (entire: vr_editor.hpp, .cpp,
  vr_editor_draw.cpp, vr_editor_interactions.cpp, vr_editor_handles.cpp,
  vr_editor_impl.hpp, design, CMake)
- `components/vr_panel/` (after `VrPanel::intersect` math is moved into
  `handle_picker`/`graph_source`; `VrPanel::draw` already dead)
- `components/rgba_shader/` (no consumers once editor/ui/poke are nodes)
- `components/text_mesh/` GL class (keep `glyph_layout.hpp` if still used
  by `text_label`; delete `text_mesh.hpp/.cpp` GL)
Files EDITED:
- `CMakeLists.txt` (drop the deleted subdirectories)
- `components/host_app/host_app.cpp`, `components/app/app.cpp` (drop
  `EditorNode` include + registration + the embedded editor JSON; load
  `editor.json` instead)
- `components/peer_core/peer_core.cpp/.hpp` (drop `EditorNode` include +
  `set_context` special-case)
- `components/rubber_band_controller/rubber_band_controller.hpp` (drops
  whichever editor helper it pulls — re-point or delete)
- `components/ray_selector/ray_selector.cpp` (delete the empty `draw_ray`
  stub)
- `components/integration_real_nodes/integration_real_nodes.test.cpp`,
  `components/port_schema_reader/*` (any editor-coupled assertions)

> Note: `ui_nodes`, `poke_button`, `poke_stick` are independent DrawFn
> producers, NOT part of the monolith. They are migrated/parked in Part
> III, not here.

---

# PART III — DrawFn full retirement

After Part II the only `out<DrawFn>`/`in<DrawFn>` producers/consumers are:
`ui_nodes` (×3), `poke_button`, `poke_stick` (migrate), and the offscreen
set `render_nodes` (×2) + `rd_renderer` (park). Then delete the typedef
and the whole bridge.

### X1 — migrate the remaining on-screen DrawFn producers
`ui_pane`/`ui_slider`/`ui_button`, `poke_button`, `poke_stick` → colored
`Mesh` + `Surface` (unlit vertex-color; reuse `vertex_color_mesh`/
`cube_node` patterns). Keep their interaction outputs (`value`/`pressed`/
`hover`/`tip_pos`). Add `draw` nodes to `control_panel.json`.
Files:
- `components/ui_nodes/ui_nodes.hpp` / `ui_nodes.cpp`
- `components/poke_button/poke_button.hpp` / `.cpp` / `.design.md`
- `components/poke_stick/poke_stick.hpp` / `.cpp` / `.design.md`
- `assets/graphs/control_panel.json` (add draw nodes + render_head chain)
- `components/host_app/host_app.cpp`, `components/app/app.cpp` (unchanged
  registrations; verify)

### X2 — park the offscreen DrawFn nodes
So DrawFn can be deleted while offscreen rework waits: unregister + drop
from the build (keep sources for the later FBO migration).
Files:
- `CMakeLists.txt` (comment out `render_nodes`, `rd_renderer`
  add_subdirectory)
- `components/host_app/host_app.cpp`, `components/app/app.cpp` (remove
  `RenderTargetNode`/`TextureViewNode`/`RDRenderer` includes +
  registrations)
- `components/render_nodes/render_nodes.hpp/.cpp` (left in tree, out of
  build — annotate "parked: offscreen leg")
- `components/rd_renderer/rd_renderer.hpp/.cpp/.test.cpp` (same)
- `assets/graphs/` (any scene referencing render_target/texture_view/
  rd_renderer — note as parked)

### X3 — delete DrawFn + the bridge; bump ABI to v8
Files:
- `components/sygaldry_endpoints/sygaldry_endpoints.hpp` (delete
  `using DrawFn`; the "draw_call" comment in natural-shape note)
- `components/sygaldry_endpoints/sygaldry_endpoints.test.cpp` (`in<DrawFn>`
  case)
- `components/signal_graph/signal_graph.hpp` (drop `DrawFn` PortValue arm;
  delete `std::vector<DrawFn> draw_calls`)
- `components/signal_graph/signal_graph_tick.cpp` (delete the `DrawFn`
  arms in `apply_value`/`read_output`, `emit_drawfn`, the
  `draw_calls.clear()` + `push_draw_calls` pass)
- `components/signal_graph/signal_graph_plan.hpp` (delete `draw_consumed`)
- `components/signal_graph/signal_graph_plan.cpp` (delete the
  `draw_consumed` build + `out_kind == "draw_call"` branch)
- `components/signal_graph/signal_graph.test.cpp` (DrawNode test, the
  `draw_calls` assertions, descriptor literal `push_draw_calls`/
  `set_drawfn_in` fields)
- `components/eyeballs_node_abi/eyeballs_node_abi.h` (ABI v8; delete
  `emit_drawfn`, `push_draw_calls`, `set_drawfn_in`)
- `components/eyeballs_node_abi/eyeballs_node_abi.hpp` (delete
  `DrawCallCtx`, the `DrawFn` branches in `v6_kind`/`push_outputs`/
  `push_draw_calls_fn`/`set_drawfn_in_fn`, descriptor wiring)
- `components/eyeballs_node_abi/eyeballs_node_abi.test.cpp` (`out<DrawFn>`)
- `components/subgraph_node/subgraph_node.hpp/.cpp` (delete
  `push_draw_calls_to`, the `DrawFn` cache arm in `push_outlets`)
- `components/subgraph_node/subgraph_descriptor.cpp` (delete
  `push_draw_calls`, `set_drawfn_in` wiring)
- `components/subgraph_node/subgraph_node.test.cpp` (DrawFn cases)
- `components/peer_core/peer_core_http.cpp` (delete the `DrawFn` visit arm
  in /values serialization)
- `components/port_types/port_types.hpp` (no change to logic; "draw_call"
  remains a valid kind only if still referenced — verify removable)
- `components/port_types/port_types.test.cpp` (drop the `draw_call` cases)
- `components/port_schema_reader/port_schema_reader.hpp` (drop
  `is_drawable`; `is_wirable` becomes trivially true — verify callers)
- `components/host_app/host_app.cpp` (delete `for (auto& call :
  g->draw_calls) call(pv);`)
- `components/app/app.cpp` (delete both `draw_calls` loops — lines ~416)
- `components/vr_editor/vr_editor_impl.hpp`, `vr_editor_handles.cpp` (the
  `draw_call` color cases — moot if vr_editor already deleted in Part II;
  otherwise drop)
- design docs mentioning DrawFn: `components/wire_mesh/wire_mesh.design.md`,
  `components/aurora_curtain/aurora_curtain.design.md`,
  `components/poke_button/poke_button.design.md`,
  `components/poke_stick/poke_stick.design.md`,
  `components/editor_node/editor_node.design.md` (deleted in II),
  `studies/component-consolidation.md`
- `components/render_region/render_region.cpp` (drop the "legacy DrawFn
  pass" comment at the clean-GL-state line)
- `components/scene_snapshot/scene_snapshot.hpp/.cpp` — **NOT** DrawFn:
  `SnapshotDrawFn` is the headless-verification typedef, unrelated. Leave.

### X4 — docs + close
Files:
- `planning/render_as_nodes.md` (mark DrawFn deleted; offscreen-only
  remainder), `planning/status.md`, `kanban/ready/conformability_executor.md`
  (rung 5 acceptance), `kanban/ready/vr_editor_decomposition.md` (closed)

---

# DEFERRED — offscreen on render_region FBO passes (later leg)
Revive the parked nodes on the new path: `render_target` = a `render_head`
with an FBO target whose color attachment flows out as a `GpuTexture`;
`texture_view`/`rd_renderer` = textured-quad nodes on the R0 texture
binding. Then re-add to the build + registrations.
Files: `components/render_region/render_region.hpp/.cpp` (FBO passes),
`components/render_nodes/*`, `components/rd_renderer/*`,
`components/rd_gpu/*` (stays a resource-holder), `CMakeLists.txt`, both
shells, affected `assets/graphs/*.json`.

---

# Cross-cutting discipline
- **Register in BOTH shells** (`components/host_app/host_app.cpp` +
  `components/app/app.cpp`) for every new node; keep the Quest
  cross-compile (`sh/build.sh`) green each step.
- **Verify per increment**: `spectator` headless EGL → POST graph JSON →
  camera → GET /screenshot → Read PNG → telegram; full host gtest
  (`sh/test_host.sh`) after each step.
- **ABI bumps**: v7 at L0 (lift metadata), v8 at X3 (DrawFn removal).
- **RT-audio safety** preserved (no syscalls/logging in the audio
  callback; lifting touches `build_plan`/`wire_plan`, not the callback).
- **Live graph-swap**: lifted-instance migration must survive the
  `migrate_graph` swap (the editor edits the graph it lives in).

# Master file index (everything implicated, by package)
- **signal_graph**: signal_graph.hpp, signal_graph_plan.hpp/.cpp,
  signal_graph_tick.cpp, signal_graph_sort.hpp/.cpp, signal_graph_migrate.cpp,
  signal_graph_serial.cpp, signal_graph.test.cpp (+ new signal_graph_edit.cpp)
- **ABI/schema**: eyeballs_node_abi.h, .hpp, .test.cpp;
  port_schema_reader.hpp/.cpp/.test.cpp; port_types.hpp/.cpp/.test.cpp;
  sygaldry_endpoints.hpp/.test.cpp
- **subgraph**: subgraph_node.hpp/.cpp/.test.cpp, subgraph_descriptor.cpp,
  subgraph_descriptor_fwd.hpp; spawner_node.hpp
- **lifting precedent**: ugens/ugens.hpp; audio_region/* (no edits expected,
  confirm); component_registry.hpp/.cpp
- **render boundary / payloads**: render_payloads.hpp; render_region.hpp/.cpp,
  render_region_nodes.hpp; color_mesh.hpp, vertex_color_mesh.hpp, sprite.hpp;
  render_nodes.* (parked), rd_renderer.* (parked), rd_gpu.*
- **editor (deleted/replaced)**: editor_node/*; vr_editor/* (all 6);
  vr_panel/*; rgba_shader/*; text_mesh/* GL; ray_selector.cpp
- **editor (new nodes)**: graph_source/*, edit_sink/*, handle_picker/*,
  wire_drag/*, slider_drag/*, dwell_delete/*, undo_node/*, palette/*
- **editor (reused nodes)**: poke_stick/*, poke_button/*, ui_nodes/*,
  grab_detector/*, grab_target/*, tethered_point/*, text_label/*,
  wire_mesh/*, rubber_band_controller.hpp
- **shells / core**: peer_core.hpp/.cpp/.design.md, peer_core_http.cpp,
  peer_core_bridge.cpp; host_app/host_app.cpp; app/app.cpp;
  integration_real_nodes.test.cpp, integration_http/*
- **every port-declaring node (L3 audit, 49 files)**: aurora_curtain,
  chladni, claude_tmux, color_mesh, cube_node, dac_node, editor_node,
  fly_camera_node, glsl_effect, hand_node, lissajous, math_nodes,
  mesh_nodes, mic_input, net_nodes, osc_node, particle_system, poke_button,
  poke_stick, ptt_gate, rd_gpu, rd_renderer, reaction_diffusion,
  render_nodes, render_region_nodes, renderer_node, sample_player, sky_dome,
  spatialize_node, spawner_node, spectrogram, speech_to_text, sprite,
  star_field, stt_whisper, sun_light, terrain_generator, text_label,
  trigger_edge, tts_local, tts_node, ugens, ui_nodes, vertex_color_mesh,
  water_surface, wav_player, whisper_node, wire_mesh, xr_sources
- **acceptance**: tree_generator/*; assets/graphs/{forest,card,editor,
  control_panel}.json + existing scenes
- **build**: root CMakeLists.txt (+ per-new-component CMakeLists.txt)
- **docs**: planning/{conformability_lifting,render_as_nodes,status,
  subgraph}.md; kanban/ready/{conformability_executor,vr_editor_decomposition}.md;
  kanban/backlog/conformability.md; this file
