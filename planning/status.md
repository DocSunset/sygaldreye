# Status

_Keep this current. Vision and slice plan: `planning/vision.md`._

## 2026-07-05 — Rungs 1–3 GREEN (builder session 1, continued)

Suite: 30 pass, 0 fail. Rung 2 (escapement): COR-1 freestanding audit
(EMPTY undefined-symbol table), hand-frozen hello-cosine movement renders
the golden take (stdlib-FFT property checks in _helpers.golden_audio_check,
negative cases verified); generator (src/generator, PFR walk at build time)
emits descriptors + C++ codecs + Python bindings + hook shells into
build/generated/; ABI audits: 10k callbacks zero RT allocs, create()-time
acquisition aborts, declared fault → value + region keeps ticking,
undeclared throw kills quarantine subprocess w/ testimony + wired restarts.
Rung 3 (crown): plan + five appliers at tick boundaries + tape reader;
render-tape output BYTE-IDENTICAL to the frozen movement.

Blessing PENDING: the 8 s take went to Travis
(documentary/media/2026-07-05-hello-cosine-first-take.wav). When he says
"blessed": commit that wav as a capture per fixtures/golden-audio.md.

Book fix (ADR-wins rule): ch. 16 stratum-3 table wrongly listed `loader`
(20 names vs its own "Nineteen" and ADR-028); removed; core-names.txt
regenerated; gates.sh COR-3 extraction fixed; COR-5.1 clause gate added.

CI is still python-only (nix flake check doesn't build ./syg) — worth
upgrading when convenient. Next: rung 4 — liveness organs (SZ-7, EXE-5,
LNG-8); read the full book first (HANDOFF: "full read before rung 4").

## 2026-07-05 — Greenfield rung 1: 15/16 green (builder session 1)

Construction began. Toolchain: root CMake + the flake (nlohmann_json;
BLAKE3 reference C impl as a pinned flake input, portable lane). `./syg`
(symlink → build/syg) implements, per HARNESS.md: encode, parse-address,
pins, hash, verify, chunk-put, naming (scripted resolution session),
connection-legal. Source layout: `src/formats/{pins,dagcbor,address,cid,
chunk}`, `src/naming/{environment,resolver,oracle,spans}`, `src/syg`.

Green: FMT-1/2/5, NAM-1.1/1.2, 2.1/2.2, 3.1, 4.1, 5.1–5.3, 6.1/6.2, 7.1.
One commit per criterion, id in message, gates green throughout.

**NAM-5.4 is the rung's one pending criterion and it is pending BY
CONSTRUCTION**: "no kind/promise lookup during tick" needs a tick to
instrument, and the tick loop is rung 2's escapement. Its test is written
(`syg tick-audit`, HARNESS.md) and goes green with rung 2's first
deliverable — the manifest extractor bins all NAM at rung 1, which
over-reaches the appendix's own rung-1 gate ("FMT-1..3") for exactly this
one criterion. Recorded here instead of STUCK.md because nothing is stuck:
the next rung's first commit closes it. No design reopened, no test
weakened.

Harness repairs made (recorded in HARNESS.md; tests strengthened, never
weakened): (1) FMT-1's differential corpus crashed json.dumps on raw
bytes — test values now cross via the DAG-JSON projection ({"/":{"bytes":
b64}}, links {"/": cid}, "/" reserved); (2) an unknown syg subcommand
(exit 2) reads as Pending so pre-written later-rung tests pend rather than
fail while their surface doesn't exist.

Next: rung 2 — the escapement (COR-1 freestanding; hand-frozen
hello-cosine movement; golden audio; kernels salvaged verbatim from
probe/components/synth_core). First milestone: render the take, send it to
Travis for blessing (fixtures/golden-audio.md). Never self-bless.

## 2026-07-05 — The guarantee package (conformance/ + BUILDER.md)

Insurance against limited time and a less capable successor: the spec is now
EXECUTABLE and the path unambiguous. conformance/: manifest.json (105
requirements / 121 criteria, extracted from the book by extract_manifest.py —
regenerate, never hand-edit), run.py (zero-dep runner; walks the twelve
rungs, reports the first unmet gate — "YOU ARE HERE"), gates.sh (law gates:
platform-branch/serializer/singleton greps, core-name manifest vs ch.16,
manifest freshness), fixtures (hello-cosine interchange + boot tape in the
now-PINNED formats, dag-cbor starter vectors, golden-audio properties +
blessing protocol). Ch.14 pins frozen (blake3-256 CIDv1, dag-cbor 0x71/raw
0x55, 256 KiB chunks, tape record grammar, percent-escaping, varint+cbor ws
framing). BUILDER.md at root: the loop, the three rules, the trap list.
Also: ADR-029 (addresses are routes from here), ADR-030 (type dissolves),
ADR-031 (derivation is the shape; commitment the act) + book-wide sweeps;
Appendix B genealogy; read-aloud revision + mkpdf --audio.

## 2026-07-04 — The greenfield session + the great consolidation

ADR-013…028 ratified over two days: per-sample islands, freezing as a
realize backend, rate-keyed execution semantics (push/pull/clocked), the
fault model (errors are values; failure is death; handling is a package),
two-lane encoding with generated codecs and open projections, op-tree
history, one-core-many-hosts (Python/JS as trampolines), three disciplines
forever, determinism classes, QoS placement + posed edges, multi-writer
(arbiter + merge-as-derivation), queries-as-graphs (core), node succession,
perpetual greenfield (locks, conformance-as-definition, core ratchet), the
self-hosting closure, and the horology: **escapement · movement · crown ·
complications** (ADR-028). Several gaps dissolved into package sketches
(transport, companion — kanban/backlog).

The book grew chs. 13–17 + the greenfield-build-order appendix and was
retrofitted throughout; it is now the SOLE design document. Superseded
planning docs (bootloader, datasets, naming, trust, rhizome, edge_executor,
kernel/lifting/render docs, subgraph, network_bridge, handoff, sprints) and
the incomplete concurrency notes were consolidated in and git-removed —
git history keeps them. AGENTS.md's architecture section now points at the
book; the probe is officially reference-and-salvage. Strangler path (CMP-8)
retired in favor of the appendix's greenfield rungs.

## 2026-07-03 — The Architecture Book (architecture/)

Eleven chapters distilling the ratified design for implementers and builder
agents: glossary (full definitions for the lexicon), overview (hello-cosine as
the running example), eight part chapters with enumerated requirements +
acceptance criteria written to become automated tests (prefixes NAM/STO/EXE/
CMP/SZ/PKG/MSH/EDR), and ch. 10 "The Law" — needs N1–N8, laws L1–L17,
protocols, and the full requirement→law→need traceability table. Lexicon now
points to the glossary for definitions. Where book and planning docs disagree:
lexicon + adr.md govern.

## 2026-07-03 — Terminology session: the ontology reaches its floor

`planning/lexicon.md` (NEW) — ratified concepts, definitions pending, organized
by **form vs function** (Kevin Austin's modular-synthesis distinction at the
level of all data and computation). The floor: one form, **data** ("just bits
somewhere; traces"); two primal functions, node (data as container) and link
(data as reference; edge/name/route/ref/address = links with derived
qualities); everything else is patching. Subsumed: entity → node, occurrence →
instance, contents → data, component → prose vestige. System definition
inscribed in vision.md ("a protocol for encoding recursive metadata and
decoding data to derive other data"). Stage 0 REFRAMED in bootloader.md: a
frozen realization of the boot graph — everything in it is ordinary nodes; the
only irreducible property is pre-existence; its build derivation is a Nix
derivation (provenance continues into the flake lock). Terminology sweep
applied across the 07-02 docs.

## 2026-07-02 — Architecture session: the data model closes (ADR-006…012)

The planning session convened by `planning/architecture_handoff.md` ran and
closed its agenda. Ratified: datasets-and-derivations as the foundation (a
dataset **is** a node; derive vs capture); naming (root+route; mutability lives
in the name — `planning/naming.md`, NEW); store-as-graph + provide/compatible
availability (`planning/datasets.md` rewritten); trust (per-peer keypairs,
advertisement-as-sandbox, graphs-vs-plugins — `planning/trust.md`, NEW);
extension points are ports, order is wiring; detachment resolved (realized views
write back through the route map; no override dataset); graph = composite entity
(topology + defaults; presets fall out). **Sygaldreye subsumes Rhizome**
(`planning/rhizome.md`, NEW; ~/agents/projects/rhizome is an abandoned probe).
Stage 0 gains the naive resolver. Handoff doc marked resolved; vision.md gained
"The deeper foundation". Not yet done: work-item planning for the migration
slices (deliberately deferred — still architecting).

## 2026-06-15 — S6 + DrawFn retirement DONE (branch `lifting-editor-drawfn`)

The arc closes. Both shells now BOOT the node-based editor (`editor.json`'s
graph, embedded in `peer_core/editor_default_graph.hpp`; the `card` subgraph
registered from its embedded definition). The VrEditor/EditorNode monolith is
DELETED, and with it `vr_editor`, `editor_node`, `vr_panel`, `ray_selector`,
`rgba_shader`, the GL `text_mesh` class (glyph layer kept), and the dead
`ui_nodes`/`poke_*`/`control_panel.json` + `rubber_band_controller` cluster
(grab/tethered/cylinder). In-headset verification was WAIVED — the goal was the
architecture (graph-native editor, no DrawFn), and it renders headless.

**DrawFn is gone (ABI v8).** The typedef, `Graph::draw_calls`, the PortValue
arm, the ABI fn pointers (`emit_drawfn`/`push_draw_calls`/`set_drawfn_in`) +
`DrawCallCtx`, and both shells' draw loops are deleted. `is_drawable` is gone;
every port is wirable. `grep -rn DrawFn components/` now shows only
`scene_snapshot`'s unrelated `SnapshotDrawFn` and the PARKED offscreen nodes.

**Parked (out of build, sources kept, annotated):** `render_nodes`,
`rd_renderer`, `glsl_effect` — they ride DrawFn; revived on render_region FBO
passes (the DEFERRED offscreen leg). `rd_gpu` stays (resource-holder).

Host gtest 43/43 PASS; Quest cross-compile (`libeyeballs.so`) green.

## 2026-06-15 — editor decomposition Part II (S3 gestures + S5 internals DONE; monolith kept; branch `lifting-editor-drawfn`)

`editor.json` now FULLY reproduces the monolith's appearance + edits. The old
VrEditor/EditorNode monolith stays LIVE, registered, and default (the gating
rule — deletion S6 is user-gated after in-headset verification).

1. SHARED LAYOUT: `editor_layout` ports the monolith's card/handle/slider
   world-space layout (default grid, 0.018 m row pitch, scalar→slider) into
   ONE tested function with identity (node id + port name) attached. Identity
   resolution = gesture nodes read this layout off the graph they observe (no
   text-array payload — the simpler/general option (b) the plan allows).
2. GESTURE NODES (S3), each a resource-holder fed controller-pose edges,
   emitting edit ops via a `GestureContext` (graph + edit queue + shared
   position overrides + sorted types) injected in pump_contexts:
   handle_picker (hover label), wire_drag (grip FSM → add_edge / card-move
   override), slider_drag (tip-x → set_param), dwell_delete (grip dwell →
   remove_node/edge), undo_node (thumb flick → restore), palette (poke →
   add_node + paging). graph_source now shares wire_drag's overrides.
3. CARD INTERNALS + PALETTE RENDER (S5): card_widgets_mesh (handles + sliders
   as vertex-colour geometry), card_labels_mesh (id labels as glyph mesh),
   editor_wires (edges → wire span), palette_mesh (panel + type rows) — the
   rendering DUAL of the gestures, all reading the same editor_layout.
4. Fixed undo: frame-diff was overwritten by param drift; now snapshots on a
   STRUCTURAL change (node/edge add/remove), the monolith's undo_json_ rule.

Verified headless (spectator): editor.json renders cards WITH handles,
sliders, labels, wires, AND the palette. Each edit gesture driven through the
live /controller path produces the right edit (connect add0.out→mul0.a;
slider set a≈+917; dwell remove victim; palette spawn add; undo 7→6 nodes),
mirrored by editor_integration round-trip tests. Monolith default graph still
renders identically.

Host: 47/47 test binaries PASS (+editor_layout, wire_drag, slider_drag,
dwell_delete, undo_node, palette, handle_picker, card_widgets_mesh,
card_labels_mesh, editor_wires, palette_mesh, editor_integration). Quest
cross-compile green. DrawFn surface untouched (Part III not started).

REMAINING: S6 deletion of the monolith (user-gated, after in-headset check).

## 2026-06-15 — editor decomposition Part II (S1/S2/S4 partial; branch `lifting-editor-drawfn`)

Built on Part I's lifting core. The old VrEditor/EditorNode monolith stays
LIVE and registered (gating rule) — these are the decomposed pieces beside it.

1. META SEAM (S1): `graph_source` (resource-holder; reads the live graph,
   publishes keys/positions/count Spans — replaces EditorNode::set_context)
   and `edit_sink` (text edit-ops → the edit queue). Wired generically in
   pump_contexts alongside the still-live editor/spawner seams. Registered
   in both shells.
2. STRUCTURED EDIT OPS (S2): `apply_edit_op` (signal_graph_edit.cpp) moves
   the editor's whole-graph string surgery into a tested op vocabulary
   (add/remove node/edge, set_param); collect_edits routes through it (full
   graphs pass through). edit_sink test covers the vocabulary.
3. KEYED-BY-ID SUBGRAPH LIFT (the rung-3 acceptance): the L1 lift keyed on
   the lifted cell; extended with a SEPARATE key source so a card keys on a
   stable id, not its moving position. Graph.lift_key (JSON top-level) →
   SubgraphDescriptor → LiftGroup.key_source. Fixed a lift-selection bug:
   a span into the lift_key port is excess-rank too and stole the lift when
   listed first (cards stacked at origin) — build_plan now skips it.
4. CARD SUBGRAPH (S4): `card_mesh` (quad at a vec3 position) + `card.json`
   (lift_key=id) + `editor.json` (graph_source → lifted card subgraph keyed
   by id → forest_draw). Spectator screenshot: 6 nodes → 6 cards, each at
   its own grid position. THE VISUAL rung-3 acceptance for keyed subgraph
   lifting. Registry now skips scene .json (no inlets/outlets) as plugins.

Host tests green (35 binaries; signal_graph 38, integration 11/8, +graph_source
3, +edit_sink 6, +card_mesh 2). Quest cross-compile green.

NOT YET (monolith kept): S3 gesture nodes (handle_picker/wire_drag/
slider_drag/dwell_delete/undo_node/palette), S5 card labels+handles+sliders
in the subgraph, S6 deletion. editor.json renders cards but has no
interaction/palette/labels yet — it does NOT yet reproduce the editor.

## 2026-06-15 — DrawFn migration leg (R0–R4 done; branch `render-as-nodes`)

Plan: `~/.claude/plans/synthetic-sprouting-lantern.md`; progress detail in
`planning/render_as_nodes.md`. Goal: migrate every DrawFn producer onto the
render-as-nodes path, then delete DrawFn. **Every visual-content producer is
now migrated**; editor rendering + offscreen + teardown remain.

- **R0 render_region capabilities**: uTime, uViewPos, GpuTexture + CPU-image
  (`ImageTex`) uniforms, `cull_front`, `additive` blend, and dynamic geometry
  (persistent slots reused via `update_verts` when topology is stable).
- **R1 mesh producers**: new `vertex_color_mesh` (lit per-vertex, wireable
  sun); `terrain` is now a geometry generator; `sky_dome`, `water_surface`
  (shader-side Gerstner waves), `wire_mesh`, `lissajous` (LineStrip), `cube`.
- **R2 instanced**: `particle_system` (sim stays; per-instance pos/size/color
  Spans), `star_field` (Points via gl_VertexID).
- **R3 effects**: `chladni`, `reaction_diffusion` (CPU sims → dynamic
  vertex-color), `aurora_curtain` (additive).
- **R4 text**: CPU-image atlas path + a `glyph::` layer; `text_label` MSDF.
- **Deleted**: `app_snapshot` (legacy snapshot tool), `mesh_render` +
  `mesh_instances` (redundant with `color_mesh`), `visual_node`,
  `particle_system_shader`.
- **Green**: host suite 32 PASS; Quest `libeyeballs.so` + on-device tests
  cross-compile clean. Each producer screenshot-verified headless.
- **Fixed**: sky `sun_dir` sign (light-travel direction) for graph-native
  lighting. **Known**: live graph-swap can leave stale render_region cache
  (fresh load correct) — invalidation is an R6 item.
- **Remaining**: R5 editor rendering (vr_editor/editor_node/poke_*/vr_panel/
  ray_selector), R6 draw-order chain + cache invalidation, R7 offscreen
  passes + **delete DrawFn**.

## 2026-06-15 — render-as-nodes (#59 render half; branch `render-as-nodes`)

Ratified with Travis then built overnight: drawing leaves C++ nodes the way
audio output left them in #58. Plan: `planning/render_as_nodes.md`.

- **#58 planar audio** committed as the branch baseline.
- **Payloads** (`render_payloads`): `Surface` (program + uniforms + pipeline)
  and `Mesh` (geometry + per-instance attribute Spans + Primitive) in the
  `PortValue` variant. No `DrawDesc` — a draw is the node plus its wiring.
- **`render_region`**: the GL boundary, the `audio_region` of graphics. Draw
  nodes enqueue (Mesh, Surface); it owns all GPU resources (program +
  geometry caches over `GlProgram`/`GlGeometry`) and is the ONLY place that
  says GL. `host_app` + `app` (Quest) drive begin_frame/issue alongside the
  legacy `DrawFn` path (bridge).
- **Nodes**: `draw` (the render endpoint, the `dac` of rendering),
  `render_head`, `color_mesh` (first shader-specific node → Surface+Mesh).
- **Instancing falls out of Span rank**: `color_mesh` fed an N×3 positions
  Span draws N instances (one `glDrawElementsInstanced`); unwired → one at
  the origin. Same node draws a cube or a forest. Effectively decomposes
  `mesh_instances`.
- **Camera-facing sprites** (`sprite` node): a quad + facing shader, alpha
  blended, instanced over a positions Span — decomposes the billboard /
  particle / star case. `render_region` injects uCameraRight/uCameraUp.
- **Verified headless** (host EGL + screenshot): single cube, 80-box forest,
  a colored grove (ground + 50 instanced trunks + 50 instanced foliage, 3
  materials, multi-draw), and 160 blended camera-facing sprites. Quest
  cross-compile clean (`libeyeballs.so`).
- **Still ahead**: glyph/text + the remaining `DrawFn` producers
  (sky/water/terrain/reaction-diffusion/…), delete the bridge + `DrawFn`,
  event-chain draw ordering, and the non-GPU rung-3 executor (subgraph
  clones / CPU cell-map). Then editor decomposition (#60).

## 2026-06-12 — the runtime grows up (two-day arc, f531b66..3bcd613)

The execution model:
- Endpoints v6 phases A+B: one `endpoints` struct, shapes in/normalled_in/
  cv_in/out/event_*, edges as literal pointers (connect/output_ptr, ABI v6),
  persisted-only serialization. Audio combiners (mix/dac/spatialize)
  MIGRATED; compat slots carry legacy-producer audio/mesh into v6
  consumers. Remaining clusters + values-map death queued.
- Text edges: PortValue carries std::string; transcripts/prompts flow as
  values. Subgraph presets take params ({"freq":880} works); inline
  subgraphs round-trip.
- Span payload: rank-≤2 float views as values. First lift: scatter →
  mesh_instances = THE FOREST (screenshot-verified; 70-trunk grove live
  on-device). Conformability design ratified incl. named axes
  (channels=map, time=scan), planar audio, per-kernel stamped lifting —
  kanban/backlog/conformability.md.
- Kernel extraction: synth_core/kernels.hpp (per-sample, dt-based, no
  absolute time); ugen shells loop kernels.
- FREEZER POSTPONED (ratified): until v6 complete + values map dead +
  spans/conformability settle.

Audio:
- AudioEngine singleton (ratified Pd model): ONE owner of audio hardware;
  output stream persists across graphs; ALL dacs sum; mic is a
  shared-ring tap (callback mode — Quest starves pull-reads); /play mixes
  engine-side, graph-independent; wav_player owns no stream.
- THE two-day silence mystery solved: uninitialized Eigen port values
  (port<T> now defaults via endpoint_default — zero vecs, identity quats).
  Also fixed en route: metro epoch trap (region migration changes a
  node's clock), AAudio teardown races (stop now waits), disconnect
  recovery, spatialize NaN/near-field guards, stale-view drone
  (wire_plan silences unwired legacy audio inputs; v6 makes it
  structural). Full mic-included playground verified 3/3 fresh boots.

Speech (native, kanban/backlog/native_speech_nodes.md):
- stt_whisper (whisper.cpp vendored, host+NDK): PTT contract, transcript
  as text edge, warm context, audio_ctx scaled to take (77 s → ~5 s class
  on Quest). tts_local (sherpa-onnx, warm, MALE piper voice, voice is an
  audio EDGE). ROUND-TRIP GATE: tts speaks, whisper transcribes it.
  On-device voice loop verified live with Travis (caption panel in VR).
  Browser/wasm rung pending. Quest mic only delivers when WORN.

Ops/robustness:
- XR-optional launch: EGL+core+HTTP before XR; offline tick when doffed;
  `setprop debug.oculus.guardian_pause 1` clears the no-controller gate.
  Launches always succeed; peer drivable with the headset on a desk.
- GET /plan (executor observability), /models upload (>3 MB POSTs still
  die — use adb push to the external files dir), MG_MAX_RECV_SIZE raised.
- Probing protocol (hard-won): always echo POST responses; missing
  /values keys ≠ zero. See block_swap_poison.md for the cautionary tale.

Open near-term: conformability lifting executor, editor decomposition
slices, device plugin dlopen crash, renders-only-when-doffed (sleep/wake
workaround), Steam Audio, JACK, bridge text mirroring across peers.

## 2026-06-10 — Slice 1 in progress

Done:
- Host build green (was broken: signal_graph/component_registry/subgraph_node
  include+link cycle; fixed with `subgraph_descriptor_fwd.hpp` out-of-line
  deleter + honest CMake static-lib cycle). All 21 graph tests pass on host.
- Host app spine: `app/spectator` rewritten around new `host_app` component.
  Live graph + HTTP on configurable `--port`: GET/POST `/graph`, `/palette`,
  GET/POST `/camera`, POST `/screenshot` (frame-synchronized PNG), `/quit`.
  `--headless` runs via EGL surfaceless + FBO; windowed mode is GLFW with
  GLES 3.2 EGL context (matches Quest) + WASD/right-drag fly camera
  (`fly_camera`, `host_input`).
- First light verified headless: sky + Gerstner water + lit cube, 0 errors.
  Telegrammed to Travis.

Bugs found & fixed (all latent on Quest as well):
1. GLSL helper prepended before `#version` (lit_shader, water_surface) —
   Adreno tolerated, Mesa rejects. Insert before `void main` instead.
2. `sky_dome` + `water_surface` never init GL when instantiated via graph
   (static `create()` factory never called by `desc->create()`); added lazy
   `init_gl()` on first tick. Suspect other visual nodes share this — audit.

Gotchas:
- Run host binaries with `LD_LIBRARY_PATH=/usr/lib` (nix loader doesn't search
  system GL); launch outside `nix develop`.
- Port 8080 occupied on Travis's box (node process) — use e.g. 8930.
- `pkill -x spectator` (-f matches your own shell).
- Some nodes compile shaders in constructors → GL context must exist before
  `HostApp::init` (`parse_graph`, `/palette` create temp instances too).

Slice 1 COMPLETE (2026-06-10, later):
- `sh/agent/`: launch/stop/screenshot/camera/graph/values/controller.sh
- GET /values port-value probe; POST /controller virtual hands
- Lazy GL init across all visual nodes (graphs parsed on HTTP thread got
  GL-less nodes — every live-added node was invisible, Quest included)
- vr_editor runs ON HOST (it was never XR-bound — only XrPosef types);
  text_mesh ported (android log → log_component)
- Agent drove the editor end to end: virtual trigger over palette spawned a
  live lissajous node. Proven by screenshot.

More bugs found & fixed:
- palette spawn inserted nodes into the EDGES array (json.rfind) — palette
  spawning never worked on any platform
- to_json emitted invalid JSON for vector ports ("light_dir":,) — every
  /graph response was unparseable by strict parsers
Logged to kanban/backlog: text_mesh glyph corruption, editor card layout.

## 2026-06-10 (cont.) — Decomposition slice: spine graphified

Architecture now (host):
- migrate_graph: live graph swaps adopt node state by (id, descriptor);
  fresh params re-applied. Editor lives in the graph it edits and survives
  its own edits. THE enabling primitive for live patching.
- Nodes: fly_camera (app renders with values["camera.pv"]), hand (HTTP-fed
  on host / XR-fed on device), editor (hand poses via ordinary edges),
  lfo/scale/add/mul, udp_send/udp_recv (loopback bridge; string params
  needed for cross-device addressing).
- host_app seams that remain C++: param queue (POST /param → deserialize on
  render thread), editor context injection (set_context/take_edit), camera
  aspect pump, final draw using camera.pv, screenshot.
- assets/graphs/*.json auto-register as subgraph node types ("plugins in
  JSON"). orbit_cam.json = adds + lfos; proven driving a cube live.
- Two-instance UDP bridge proven: B's cube bobs on A's lfo (channel 0,
  port 9100, ~1 frame lag).
- parse_graph accepts standard (whitespace) JSON now.
- "add" node doubles as a constant source (a unwired, b=value) — inlet
  fan-out inside subgraphs uses this trick; a dedicated const node would
  read better.

## 2026-06-10 (cont. 2) — first node decomposition shipped

- sky_dome → sky_dome (gradient+sun) + star_field node; assets/graphs/sky.json
  recomposes them. Duplicate-name inlets fan out one external port to several
  inner ports (no passthrough node needed).
- Bugs: ABI setters matched only display names (spaces) while params match
  field names — edges using field names silently no-oped (now both accepted;
  kanban: canonicalize). star_field xyww depth==1.0 failed GL_LESS — stars
  were likely never visible on Quest either; drawn depth-off now.
- Night-sky screenshot proves the whole chain: subgraph type from disk,
  edge-driven inlet fan-out, stars from the split-out node.

## 2026-06-10 (cont. 3) — UI as graph nodes

- ui_slider/ui_button/ui_pane (components/ui_nodes): widgets are nodes;
  hand ray + trigger arrive via edges; values + draw calls flow out.
  Slider drag state is node-internal → survives migration. VrPanel does
  the ray→uv hit math.
- assets/graphs/control_panel.json: pane + 2 sliders as a subgraph plugin
  (ray/trigger inlets fan out; value outlets). Agent dragged sliders via
  controller.sh; values verified through smooth → cube.scale.
- math primitives: const, sub, div, phasor, smooth join lfo/scale/add/mul.
- Path to editor-as-subgraph: palette = ui_pane + per-row ui_button grid;
  node cards = panes + port ui_buttons + ui_sliders; wires need a
  wire-render node + drag state machine node. Text labels need text_label
  on host (text_mesh already ported — small lift). The C++ vr_editor
  remains as scaffolding until those exist.

## 2026-06-10 (cont. 4) — RD split: textures flow over edges

- rd_gpu (sim) ported to host + registered both platforms; rd_renderer
  lazy-creates. sim.concentration → view.texture proven (Gray-Scott
  patterns on host). Any texture-input node can now consume the sim.
- GL state discipline lesson for ALL FBO nodes: own VAO, save/restore
  framebuffer + viewport, never bind FBO 0 (headless renders into an FBO).
  rd_gpu violated all three; fixed. Float32 textures need NEAREST in core
  GLES3 (Adreno extension hid this).

## 2026-06-10 (cont. 5) — the graph grows itself; decomposition pause point

- text<"name"> string params (serialize/deserialize/schema kind "text").
- text_label on host, refactored onto text params (custom serializers
  deleted). Labels render clean — the glyph-corruption kanban bug is
  isolated to vr_editor's own draw path, not text_mesh.
- spawner node (trigger edge + type param → graph edit). A palette of
  ui_pane + text_labels + ui_buttons + spawners — pure JSON, zero new
  C++ — added a lissajous node to its own graph when pressed.

### Where decomposition pauses, and why

The pattern is proven and repeatable: producer/consumer over typed ports
(sky→stars, rd sim→renderer), subgraph plugins (.json as node types),
UI-as-nodes, graph-edits-as-node-output. What remains is either:
1. MECHANICAL repeats (water mesh/shader split, aurora curtains,
   particle emitter/renderer) — valuable but no new architecture; safe to
   do anytime, ideally after canonical port names land.
2. DESIGN-GATED work that should NOT be improvised:
   - context injection for nodes inside subgraphs (spawner/editor in a
     subgraph never gets graph/registry — needs the executor's context
     story, not more special cases in host_app)
   - string EDGES (events/JSON through ports) vs string params
   - audio decomposition (needs host audio output + thread regions —
     the edge/executor design's core case)
   - registry-driven UI (palette enumerating types = graph generated
     from app state — wants a principled reflection mechanism)
3. HARDWARE-GATED: Android shell parity + Quest validation (headset
   session), browser peer.
Category 2 is the edge/executor design doc (slice 3) — Travis ratifies.
Decomposing further without it would bake in seams we'd have to unwind.

## 2026-06-10 (cont. 6) — mechanical splits done (Travis green-lit)

- aurora → aurora_curtain node (color/motion/placement patchable) +
  aurora.json. 'aurora' type comes from JSON on host; C++ monolith stays
  Android-only until APKs ship assets (kanban-worthy cleanup).
- particle_system: emit pos/velocity/gravity/color as ports. Fixed:
  graph-spawned particles were invisible (zero-init EmitterParams).
  split3/join3 vec3 plumbing primitives added.
- water: spectrum (wavelength/choppiness/amplitude) as inputs w/ dirty
  rebuild. Calm→storm live. (True mesh/shader node split intentionally
  skipped: waves are computed in the vertex stage — splitting would force
  a heightfield-texture redesign; deferred to edge/executor era.)
- Demos: lfo-breathing aurora; orbit_cam.json reused to orbit a spark
  fountain; sea-state flips via /param.

Next session: edge/executor design doc draft + network bridge walkthrough
(see memory: followup-network-bridge-design).
- string PortValue/params (UDP host addressing, labels, JSON events)
- editor deep bug hunt: wire-drag (grip), sliders, dwell-delete, undo
- Android app.cpp: adopt migrate_graph + hand/editor nodes (currently still
  the old C++ editor member; works but diverges from host architecture)
- Quest on-device validation pass
- update planning/vision.md slice plan as slices merge


## 2026-06-10 (autonomous run while Travis away)

Worked the agreed list top-down; all six items done:
1. planning/edge_executor.design.md (taxonomy/regions/context — 5 questions
   to ratify) + planning/network_bridge.md (walkthrough he asked for).
2. Editor interaction hunt — 4 bugs: stale card model (EditorNode never
   called on_graph_changed), BARE-HOVER DELETE (1 s gaze = node gone; now
   grip-gated), wire-drop first-in-radius (now nearest), sliders stuck on
   0..1 (schema now carries min/max). All verified via virtual hands.
3. Backlog cleared: MSDF screen-range formula fixed (glyph corruption
   dead; '[' glyphs were thumbs, misdiagnosis), editor cards now an
   eye-level 4-wide grid, cube ambient 0.1→0.25.
4. Tests: +8 (ui widget math headless; text round-trip; subgraph instance
   migration; two-level nested inlets). 33 total, all green.
5. Design docs for the 10 new components.
6. Android parity build-only: full node vocabulary registered + migrate on
   swap; rig NOT wired, nothing flashed — needs headset session.
Legacy snapshot exes broke on the sky decomposition (fixed minimally);
kanban proposes deleting that system — wants Travis's nod.

Awaiting Travis: edge/executor ratification (5 questions in the doc),
bridge conversation, port-name canonicalization nod, snapshot deletion
nod, headset session for device verification.


## 2026-06-10 (second autonomous run — GL graph arc)

Nods executed: canonical port names everywhere (dual matcher deleted);
bespoke snapshot exes + sh/snapshot.sh deleted.

New port types & nodes (ABI v5):
- DrawFn through edges; edge-consumed draws skip the screen pass.
  render_target (draw+pv → texture), texture_view (texture → screen).
- glsl_effect: fragment shader BODY in a text param (uTex/uTime/uA..uD),
  live-recompiled, typo keeps last good program. Shader→shader chains in
  pure JSON proven (warp + chromatic grade over a lissajous texture).
- MeshPtr (shared CPU TriMeshData) through edges. mesh_grid →
  mesh_displace (GPU texture read BACK to CPU, vertex displacement,
  smooth normals) → mesh_render. Proven: living Gray-Scott island.
- udp_send 'host' text param (cross-device ready); hsv_color, time nodes.
- sh/test_host.sh runs all 14 host test binaries (caught a stale test).

Parser hardening: string values may contain commas/braces (GLSL in
params); brace counters skip string literals; text params escape \n.

Next candidates: feedback-loop textures (effect self-input), mesh
generators (sphere/tube), render_target auto-sizing, palette buttons for
the new node families, Quest session for the whole GL arc on device.


## 2026-06-10 (third autonomous run — feedback, primitives, Quest prep)

- Texture feedback WORKS by design: topo_sort appends cycle members, the
  values map persists across ticks → back-edges deliver one-tick-delayed
  values. Locked with a self-edge integrator test. glsl_effect ping-pongs
  its targets (self-sampling is undefined GL) and gained uTex2 for
  feedback mixing. Demo: lissajous light-trails via fb.texture→fb.texture.
- Math primitives now cover all mathematical port types:
  scalar (lfo scale add mul const sub div phasor smooth hsv_color time),
  vec3 (split3 join3 vadd vscale vlerp vdot vcross vlen+normalized),
  quat (quat_euler quat_mul quat_rotate quat_slerp),
  mat4 (trs mat_mul; fly_camera emits view/proj/pv).
- Geometry: generators mesh_grid/sphere/box/cylinder; deformers
  mesh_ripple/twist/transform + texture-driven mesh_displace; mesh_render.
- CONFESSION + fix: Android 'green' checks since the repo rename were
  false — stale build/android cache failed configure instantly and my
  lowercase grep missed 'CMake Error'. Clean rebuild surfaced exactly one
  real error (app_snapshot star_count). Lesson recorded: trust exit codes,
  never grep for failure.

### Quest session checklist (Travis incoming)
1. APK packaged at build/apk/eyeballs.apk (74 MB, all new node types
   registered, migration on swap). `sh/run.sh` = install + launch + logcat.
2. Travis dons headset BEFORE launch verification (frame loop needs HMD).
3. Verify in order: default scene renders → /graph GET over WiFi (mdns
   'eyeballs' or device IP :8080) → POST a small patch (lfo→cube.scale) →
   palette shows new families → GL arc on Adreno: render_target,
   glsl_effect chain, mesh_displace readback (watch for driver quirks —
   this is the highest-risk area) → udp bridge desktop⇄Quest using the
   new host param (desktop IP).
4. Known not-yet-on-device: hand/editor-node rig (C++ editor still wired);
   assets/graphs/*.json not packaged into APK (no orbit_cam/sky/aurora
   types on device yet — flag if wanted for the session).


## 2026-06-10 (voice loop — PC half COMPLETE)

speech → whisper → graph → Claude → speech, certified hands-off:
espeak wav → companion /transcribe (faster-whisper, venv) → /param
{"node":"claude","params":{message,seq}} → claude_tmux node → tmux session
(claude --settings voice_hooks.json, cwd companion/claude_vr) → reply →
Stop hook speak_last.sh → /tts (espeak-ng) → /tmp/tts_last.wav.

Run order: sh/agent/launch.sh → post graph w/ claude_tmux node →
companion/.venv/bin/python -u companion.py --host 127.0.0.1 --port 8930
(pidfile /tmp/companion.pid; kill via pidfile — pkill patterns self-match
our own shells!).

Gotchas burned into code/docs:
- Hooks in project .claude/settings.json await interactive trust; pass a
  NON-project file via --settings to fire immediately.
- Stop hook payload includes last_assistant_message — no transcript
  parsing needed.
- TUI eats keys ~4 s after spawn (node defers first delivery).
- /param (and any new route) must compact_json its body.

### Remaining for the Quest session (device half)
- Existing on device: mic_capture, push_to_talk, speech_to_text node
  (already POSTs wav to companion /transcribe!) — record+send may nearly
  work already; multi-take append + erase + 'bip' feedback need a
  ptt_recorder evolution.
- Missing: A/B/X/Y button bindings in input.cpp (+ controller node button
  outputs + app pump); wav_player node (own AAudio stream) + /play route
  so replies sound IN the headset (companion --play-url points there).
- Travis's flow: hold X = record (bip), repeat takes; Y = send; A = erase.


## 2026-06-10 (voice loop — device half build-ready)

Travis's flow → implementation:
- hold X (bip!) speak, release (bip), hold X again, yap, release —
  lc.btn1 → stt.record; takes ACCUMULATE in PushToTalk's buffer;
  stt.bip → speaker.bip (1100 Hz start / 700 Hz stop).
- Y = send → lc.btn2 → stt.send → POST wav to companion /transcribe →
  whisper → /param to PC claude_tmux node → tmux Claude session.
- A = erase → rc.btn1 → stt.erase.
- Replies: Stop hook → companion /tts (espeak) → POST headset /play →
  param-inject into 'speaker' wav_player node → AAudio in-ear.

Quest session steps:
1. Start PC: sh/agent/launch.sh; post graph w/ claude_tmux node;
   companion ... --play-url http://HEADSET_IP:8080/play
2. adb install APK (packaged ✓); Travis dons headset; sh/run.sh.
3. POST companion/quest_voice_graph.json (PC_IP swapped in) to headset.
4. Speak. Verify bips, transcripts in companion log, replies in ears.
Risks: X/Y/A/B bindings unverified on device; AAudio capture+playback
concurrency; mic permission prompt on first run (RECORD_AUDIO).

Upgrade path noted: piper TTS (espeak is robotic), streaming STT,
hook filtering (skip tool-use-heavy replies).


## 2026-06-10 (Quest session, part 1 — live with Travis)

VERIFIED ON DEVICE: app installs/launches/renders; /graph /param /play
/values all work over WiFi; wav_player speaks TTS into the headset
(Travis: 'I heard that! Too cool!'); mic captures at 48k when worn;
full take→whisper chain reached 'transcript: You' before the wav-header
and buffer-growth bugs were fixed.

OPEN when Travis returns (headset on head!):
1. Full voice loop with real sample rate + fixed buffers (expected ✓).
2. X/Y/A/B buttons: lc.btn1 stayed 0 under hold — binding bug OR focus
   loss. Re-test worn; if still dead, dump xrSuggestInteractionProfile
   result and check trigger still works (whole-suggestion rejection).
3. Re-add btn edges to stt once buttons work (params were fighting the
   edges — see commit note).
4. GL arc on Adreno (render_target/glsl_effect/mesh chain) untested.
5. render-over-budget warnings (~10-12 ms) with voice graph — profile.

Lesson burned in: check VR FOCUS before debugging device sensors — the
OS silently denies mic/trackers when the headset isn't worn
(MemoryBroker app-ops lines in unfiltered logcat are the tell).


## 2026-06-11 (Quest session part 2 — THE LOOP IS CLOSED)

Travis, in-headset, conversing by voice with a Claude Code session:
hold X (bip) → speak → release (bip) → Y sends → whisper → claude_tmux →
Stop hook → espeak → his ears. His first self-driven exchange:
'This is a test. Do you copy, Claude?' → replies flowing.

Found/fixed live: headset must be WORN (OS denies mic unworn — the
phantom); X/Y/A/B buttons worked all along (he couldn't see my prompts —
hence the TTS-comms memory); UX screenshot testing had evicted the PC
claude node (transcripts fell into void); Enter-race on busy TUI
(paste→0.4s→Enter). Open: garbage floats in recordings (kanban, defer to
ring mappings); GL arc on Adreno still undemoed; editor-UX (ray, labels,
movable cards) shipped but not yet toured.


## 2026-06-11 (Quest session part 3 — the other Claude has eyes)

- WiFi adb live: 192.168.0.18:5555 (no cable needed).
- Device GET /screenshot: left-eye PNG via renderer post_resolve_hook
  (MSAA fbo unreadable; mid-pass manual resolve still read white on
  Adreno; post-resolve point is the only safe one). http_server made
  binary-safe (%.*s truncated PNGs at first NUL).
- companion/claude_vr/CLAUDE.md: the LLM user guide (speak short, no
  markdown; recipes for seeing/inspecting/editing the world).
- The voice Claude flagged an injected capability brief as possible
  prompt injection (kept that instinct!), Travis vouched, it looked
  through his eyes and described his view aloud. Permissions for its
  senses pre-authorized in voice_hooks.json.


## 2026-06-11/12 (overnight autonomous arc — guiding star ratified)

Travis ratified the GUIDING STAR (vision.md): any restart needed to land
a change is a failed test revealing a bug; restart-failures get logged in
kanban/backlog/live_update_gap.md with their "what would have needed to
change" answer. Headset dead + Travis asleep → ~20 h autonomous window.
Agreed arc: plugin proof → painkillers → executor 2-3 → CORE UNIFICATION
(host+android = two thin shells over one portable core) → hot-reload →
worker → audio+dac → net mappings (WebSocket transport) + two-peer demo →
companion-as-subgraph (+MeloTTS) → BROWSER PEER skeleton → editor
recomposition (stretch).

Done so far tonight:
1. LIVE PLUGIN LOOP PROVEN (host): poke_stick (components/poke_stick,
   self-contained GL, SYGALDREYE_PLUGIN export guard) compiled via
   companion/compile_node.py --target host, POSTed to /plugins on the
   RUNNING spectator, dlopen'd, wired to hand_r by live graph edit;
   renders, tip_pos flows, gold on trigger. Restart-failures found:
   compile_node.py include drift (gpu_texture/tri_mesh missing — list
   must come from the build system), host_app had NO /plugins route
   (added; unification fuel).
2. Painkillers: input.cpp ray uses /input/aim/pose (was grip); vr_editor
   seeds SliderWidget.value from node params on rebuild (display snap-back
   fixed, verified by screenshot); slider hit test picks single nearest
   track (±0.009 m) — no more multi-slider sweeps. Device verify pending.
3. Executor step 2: signal_graph_plan — TickPlan built lazily per graph;
   true edges = cached PortValue* (zero per-tick lookups); DFS back edges
   reified as z⁻¹ DelayMappings (optional<PortValue>, empty until first
   capture = certified tick-1 semantics preserved; self-edge test still
   green). serialize_graph emits derived "mappings" array (z1 entries),
   parse ignores it. 25 signal_graph tests pass.
4. Executor step 3: BangField ABI (kind "bang", event rate, 0/1 copy +
   producer-resets discipline, end-to-end test); event_queue component =
   THE queue mapping (MPSC never-drop, gtest incl. 4-thread race);
   take_edit() RETIRED (editor/spawner set_context now takes
   EventQueue<std::string>*; host shell drains; param queue unified on
   EventQueue). seq-bump retirement deferred: blocked on text events
   (design open question). All 25 host test binaries green; Android
   builds clean.

Next: core unification (task #28). Both shells duplicate: HTTP routes
(host now has /plugins too, app.cpp has /meta-graph host lacks, etc.),
param queue (android still hand-rolled), default graph, registry
population, screenshot capture, edit pumps. Travis: "two build targets
for the same exact application, at most differing in available-node list
and default startup graph."


## 2026-06-12 (overnight, continued — bridge + companion-as-subgraph)

5. HOT RELOAD (live_update_gap #3 ✅ host): re-register retires old entry
   (handle never dlclosed while registry lives); POST /plugins auto
   re-parses the running graph (params carry, untouched types migrate).
   Demo: poke_stick recolored grey→red on the RUNNING app by POSTing a
   recompiled .so. GOTCHA: plugins need -fno-gnu-unique or glibc unifies
   descriptor statics process-wide and reload silently no-ops (real
   2-version .so tests in component_registry).
6. WORKER REGION (executor step 4 ✅): components/worker; claude_tmux's
   system()/sleeps off the render thread; shared_ptr atomic results.
7. NET MAPPINGS v1 (executor step 6 ✅ value-rate): /ws WebSocket upgrade
   in http_server + ws_link client; POST /peer fetches a provider's
   advertised types → proxy descriptors "type@host:port" (remote_node,
   slot trampolines). Spawning a proxy spawns the REAL node in the
   provider's live graph; inputs coalesce forward, outputs mirror back
   per frame. DEMO: consumer cube driven by provider lfo across two
   processes; params forwarded live (freq 0.2→3 obeyed).
8. COMPANION AS SUBGRAPH (✅ host-proven): whisper_stt + tts nodes
   (worker region) + POST /transcribe core ingress (symmetric with
   /play→"speaker"; targets node id "stt"). MeloTTS branch merged,
   3.0G venv moved to companion/.venv-melotts, tts_cli.py/transcribe_cli.py
   wrap them. Proven: wav → /transcribe → whisper → claude_tmux tmux
   paste; message → tts → MeloTTS wav → /play. speak_last.sh speaks via
   the graph now (companion.py = fallback); quest_voice_graph points at
   PC:8930/transcribe; host default graph carries stt/claude/speak.
   Kanban: tts_warm_process (cold loads slow).

Deferred: audio region (step 5) — least verifiable with the headset dead.
Next: browser peer skeleton (#34).

## 2026-06-12 ~08:30 (overnight arc PAUSED — budget)

Credit check: 84% of the monthly pool spent (CA$8.47/10, resets Jul 1).
Browser peer (#34) is the most expensive remaining task (emscripten
bring-up, many compile iterations) — deferred rather than risk leaving
Travis without interactive budget for 19 days. Audio region (#31) also
parked (needs the headset anyway). Editor recomposition (#35) untouched.
All overnight work committed (95b57c2..93f1067). Device replays pending
when the headset charges: painkiller verify, plugin/hot-reload on
Adreno, headset-as-third-peer, graph voice loop end-to-end.

## 2026-06-12 (session reset — browser peer LANDED, #34)

9. BROWSER PEER (✅ skeleton): app/web compiles the graph core to WASM
   (emcmake standalone project; -sFULL_ES3/WebGL2; pass
   -DSYGALDREYE_EIGEN_DIR/-DSYGALDREYE_BOOST_DIR). Renders the default
   scene in a canvas (verified headless chromium + swiftshader);
   mouse-look + WASD; ?peer=ws://host:port/ws auto-connects (ws_link
   gained an Emscripten transport; RemotePeer gained non-blocking
   request_types/types_ready), ?graph= applies once remote types exist.
   PROVEN: browser registered the native peer's 69 types and spawned a
   real lfo into the RUNNING native graph (r1_lfo visible in /graph).
   Gotchas burned in: headless browsers throttle rAF → network pump
   runs on emscripten_set_interval; ninja re-runs cmake WITHOUT caller
   env → include dirs persist as cache vars; NDK-gtest blocks and
   GLESv2/v3 link lines needed EMSCRIPTEN/EXISTS guards (committed).
   Run it: python3 -m http.server in build/web, open web_app.html
   (?peer=… for the bridge). Touch controls + value-mirroring polish
   still open.

Remaining from the arc: audio region (#31, needs headset), editor
recomposition (#35, stretch). Commits through 61bba5b; 25 suites green;
Android clean.

## 2026-06-12 (arc COMPLETE except audio — #35 landed)

10. POKE INTERACTION AS GRAPH (✅, first editor-recomposition slice):
    poke_button node (AABB hit test vs probe point; idle/hover/fire
    colors; bang output) + spawner gains bang<"spawn"> (bang→scalar is
    type-illegal by design — events stay events). Demo: hand pose →
    poke_stick → tip_pos → poke_button → bang → spawner: poking the
    button spawned cube_1781181593 into the running graph. Zero editor
    C++; both nodes plugin-shippable.

Every overnight-arc task is done except the audio region (#31 — needs
the headset). Headset replay list, in order, for the next session:
1. Painkillers verify (aim ray, sliders).  2. Plugin + hot reload on
Adreno (compile_node.py --target android; NDK flags unverified).
3. Headset as third peer (POST /peer both ways; controllers driving
host/browser scenes remotely).  4. Voice loop through the graph nodes
(warm-process fix first or replies will lag).  5. Poke stick + button
in actual VR.

## 2026-06-12 (audio: ears + the block region — executor COMPLETE on host)

11. SPECTROGRAM (the agent's ears): components/spectrogram — rolling
    STFT (radix-2, Hann, log mag) into an R8 ring texture; osc test
    source. Chain: audio edge → spectrogram → texture_view →
    /screenshot. Proven: lfo-swept tone draws its trajectory.
12. AUDIO REGION (executor step 5 ✅ → ALL SIX STEPS NOW LANDED):
    audio_region schedules dac-upstream nodes in the audio callback;
    SDL2 audio_output for host AND web (Web Audio in the browser; AAudio
    untouched on device); latch/snapshot/ring boundaries reified;
    offline pump for headless. Live host demo: sweep audible AND visible
    (lfo→latch→osc→dac + osc→ring→spectrogram). Web: real Web Audio
    stream opened headlessly, ring-fed spectrogram columns drawn.
    Device got the region through peer_core for free; osc/dac/
    spectrogram registered all three targets. kanban:
    audio_region_followups (RT purity, autoplay gesture, synth
    migration, mixer).

## 2026-06-11/12 (UGEN DECOMPOSITION — phases 0–3 landed in one arc)

13. ENABLERS: subgraph inlet/outlet kinds derive from inner schemas
    (audio-outlet presets join the block region — THE unlock for synth
    subgraphs; editor wire legality tightens free); stereo through
    edges/dac; PortValue latches (vec3/quat cross frame→block); event
    queue crossings both directions; device presets via
    internalDataPath/graphs + sh/push_graphs.sh; POST /plugins ships
    JSON presets (sniffed; optional top-level "name" names the type).
14. UGENS (components/ugens, all targets): noise(white→pink),
    adsr(bang-triggered, envelope-as-audio), vca, mix, biquad node,
    delay, shaper, sample_hold, slew, grain_cloud, metro; osc gains
    wave shapes. biquad band_pass fixed to unity peak (test only began
    running when the lib went cross-target — caught the constant-skirt
    Q-gain form).
15. PRESETS: all seven synths are now assets/graphs/*_synth_g.json
    subgraphs over the ugens — chime proven note-by-note (3 partials at
    exact ratios on the spectrogram), rain/engine/atmos/creature/fire/
    water auditioned + screenshot-verified. Live-editable, plugin-
    shippable, cross-target.
16. SPATIALIZE: mono in + world pos + listener pose (latched) → stereo
    out + per-ear RMS; verified by ear-level divergence as a chime
    orbits. AudioScene's job is now topology. sample_player replaces
    wav_player's machinery (worker loads, /play contract intact, speech
    spectrum verified). Legacy synths/audio_scene/wav_player retire
    after device A/B — kanban ugen_followups.

Stale-instance trap struck TWICE more (old binary on 8930 masquerading
as fresh → phantom 'parse failed'). pkill -9 '[s]pectator' + port-free
check before EVERY launch; sh/agent/launch.sh for backgrounding (bare
'&' children die with the tool shell).

## 2026-06-12 (editor decomposition begun — slice 1: wires as graph content)

1. WIRE_MESH NODE: editor publishes wire endpoints as an N×10 span
   {from.xyz, to.xyz, rgba}; wire_mesh tessellates the beziers and
   draws GL_LINES with per-vertex color. Wire *look* is now a node you
   can swap live; VrEditor::draw lost the wire pass (PrimBatch line
   runs deleted).
2. DEVICE EDITOR UNIFIED: app.cpp's embedded VrEditor is GONE. The
   device default graph carries ctl_l/ctl_r (controller sources) →
   editor → wires, identical shape to host. peer_core's pump_contexts
   already injected editor context generically, so the shell shrank by
   ~40 lines. CONSEQUENCE: a pushed scene must include the editor rig
   (ctl_l, ctl_r, editor, wires + edges) or the device loses editing
   until the next push — editor is content now, not chrome.
3. Verified: host screenshot shows kind-colored beziers between hand
   cards and editor + grey editor.wires→wires wire; device /values
   shows editor.wires span[11x10], both render DrawFns live. In-headset
   interaction check pending (Travis).

Next slices (vr_editor_decomposition.md): hover labels, sliders via
structured edit ops, cards+palette via lifting, gestures, delete VrEditor.
