# Architecture planning handoff

Written 2026-07-02 at the end of a design session that began as "assess migrating to a
self-bootstrapping graph executor" and converged on a far-reaching architecture for the
whole system. This document is the entry point for a dedicated architectural planning
session: it captures what was decided, why, what remains open, and where everything
relevant lives in the repo.

Read order: this doc → `planning/bootloader.md` (the converged design + original
assessment appendix) → `planning/datasets.md` (store design) → the referenced code and
docs as needed.

## Origin and arc

The initiating goal (Travis): every non-frozen sygaldreye program starts from a
completely portable graph-executor bootloader; its first stage is the only native C++
outside the graph execution system and is the same on all platforms; all
platform-specific behavior lives in platform-specific nodes and platform-selected
graphs. A forcing function to route everything into graph nodes, followed by a second
step of properly factoring the nodes.

The discussion moved through five convergences, each subsuming the last:

1. **Pacer inversion → executor nodes.** The shell-driven main loops (XR frame loop /
   GLFW / rAF) invert: a node owns a subgraph plus the execution context that ticks it
   (thread, subprocess, async event loop). The audio region is the existing prototype.
   Executors own pacing and the platform resource; inner node families (dac/adc,
   xr_session, draw) bind through a per-flavor contract.
2. **Three-level model.** Application graph (authored, platform-agnostic) → engine
   graph (examines app graphs, constructs execution graphs) → execution graph (lowered
   artifact: explicit executor containment, platform nodes, boundary adapters). A
   self-hosting compiler pipeline. "Containment vs inference" dissolved: inference is
   the app-graph view, containment the execution-graph view; lowering maps them.
3. **Roles, not types — a reflective tower.** App/engine/execution are roles in a
   compilation relationship, not kinds. Every graph is defined (as data), compiled by
   an engine graph, realized as an execution graph — including engine graphs. Grounds
   at stage 0 (naive C++ executor). Laziness rule: a level's engine graph is
   instantiated only when someone edits at that level; the tower is potential, not
   resident.
4. **Datasets and derivations (the foundation).** The graph was never the fundamental
   object. A dataset is; a pipeline is a derivation from datasets to datasets;
   provenance-or-fork is the law relating derived data to sources. A graph is a
   dataset an executor can run. Compilation, freezing, analysis, ML training are all
   derivations. Travis confirmed this anticipates his unstated vision for datasets
   generally (additive synth, atomic decomposition analysis-resynthesis, video
   compositing, projection mapping, ML workbench — validation targets, not
   requirements).
5. **The store.** Content-addressed immutable objects + mutable refs; symmetric
   protocol, asymmetric pinning (Linux archives, Quest/browser cache); the mesh as a
   single PSK trust domain; access via store nodes. See `planning/datasets.md`.

## Settled decisions (with the reason)

- **Executor owns pacing + platform resource; contracts for inner node families.**
  Matches existing code (AudioRegion/AudioEngine own thread/device; DacNode bridges
  in). Retires the three hand-rolled versions: RenderRegion singleton,
  `pump_xr_sources()`, dac's `AudioEngine::instance()` reach.
- **Spawn AND exec both supported** — two call sites of the one primitive "replace the
  subgraph of slot X, migrating state" (exec = swap own slot; spawn = child slot +
  park with restart policy). Per-stage policy. Stage 0 always spawns stage 1 and
  remains resident: near-zero cost, and it is the one tower level that cannot be
  edited away.
- **Stage-0 platform independence is the invariant**, not graph uniformity.
  Per-platform boot graphs and hard-coded per-target registries are acceptable; what
  is forbidden is stage-0 code branching on platform. Entry symbols (android_main /
  main / emscripten) are accepted as <10-line trampolines that hand an opaque
  platform-context stash to the bootloader.
- **Registration = linkage** via a CMake-generated per-target registration TU
  (deterministic, loud link failure, readable capability manifest). Runtime inspection
  of linked nodes = reading ComponentRegistry's map (already surfaced as the palette).
- **Edits to realized artifacts: provenance with detachment.** An execution-graph edit
  either becomes a pass appended to the producing pipeline (persists across
  re-lowering) or forks provenance (detachment recorded; upstream no longer clobbers).
  A graph with no upstream definition is an app graph with identity compilation.
  Travis confirmed this is exactly his vision for datasets generally.
- **Capability packages**: an executor region is its own app/engine/execution world —
  vocabulary (platform nodes) + passes (spliced into the engine pipeline's declared
  extension points: recognize region, construct context, choose adapters) + machinery
  (C++ context internals). Splicing a package in is itself an edit to the engine
  graph, driven by environment observation.
- **Capability-driven placement**: when a package is absent locally, lowering may
  place that region on a remote peer advertising the capability (net adapters +
  remote-peer machinery exist). Cross-network placement is a fallthrough case of
  lowering, not a feature. Travis: emphatically yes.
- **Dataset payload, not graph payload**: the new port-system value is kind + content
  + provenance reference; graph is the first kind. Provenance records
  content-hash-friendly from day one (memoization later without retrofit).
- **Two pipeline modes on existing executors**: streaming (live frame/block tick) and
  derivation (run-to-completion with provenance recording — a worker-region
  execution; the worker region is the build system).
- **The store is a capability package, not stage 0.** The embedded boot graph lets a
  peer boot before or without a store.
- **Frozen programs bypass the bootloader by design** — freezing is the extreme point
  of the same spectrum: realization to C++ with provenance for unfreezing
  (`kanban/backlog/freezer.md` already carries the provenance doctrine).

## Open questions (the planning session's agenda)

1. **Provenance/detachment semantics in detail.** Promoted from editing rule to the
   data model of the whole platform. Must be settled before execution graphs become
   editable and before the first non-graph dataset kind.
2. **Identity-preserving lowering.** Deterministic lowering emitting app-node-id →
   execution-node-id maps so state survives re-lowering (generalizes today's
   swap+migrate). Named the main engineering risk.
3. **Engine pipeline extension points.** What they are, how packages declare passes,
   pass ordering. Prerequisite for the second capability package (XR).
4. **Trust domain**: confirm single-trust-domain + PSK for the foreseeable horizon.
   Note: capability placement makes graphs remote code execution by design — the mesh
   boundary is a security boundary. Today's HTTP surface is unauthenticated LAN.
5. **Store formats**: hash function + object header — trivial now, migration-painful
   later.
6. **Ref history retention**: recommended forever on archive peer, bounded on caches.
7. **Engine-graph debuggability**: purpose-built probes early; pull-observability
   (endpoints v6 phase D) is the substrate.
8. Allocation discipline: no resource acquisition in node create(); first-tick lazy
   init; fail loud when ticked without context. (Agreed in principle; needs auditing
   across GL nodes.)

## Migration path (survived every design revision unchanged — deliberately a strangler)

1. Generated per-target registration TU — kills the three hand-maintained lists
   (host_app.cpp: 106 register_builtin calls; app.cpp: 92; web_main.cpp: 41).
2. Wrap the existing C++ `build_plan` pipeline as a single `lower` node; initial
   engine graph = receive → lower → realize. Semantically the current system,
   restructured into the new shape.
3. Retrofit AudioRegion as the first capability package (vocabulary: dac/adc; passes:
   today's dac-closure inference; machinery: existing thread/device code). Proves the
   abstraction where it already exists in disguise; no XR risk.
4. Stage-0 extraction from PeerCore + trampolines; host first.
5. Declare engine pipeline extension points.
6. Quest package: xr executor absorbing xr.cpp + XrSessionObj + FrameLoop +
   Renderer/EglContext/EyeSwapchain; input via the xr contract (delete
   pump_xr_sources); retire Scene; mdns/http as nodes.
7. Web onto the same bootloader.
8. Factor passes out of the `lower` monolith into engine-graph vocabulary; settle
   provenance/detachment before opening execution graphs to the editor.

A useful output of the planning session would be kanban items for slices 1–4.

## Codebase context (surveyed 2026-07-01, commit 313b50e)

### What already aligns with the target

- `PeerCore` (components/peer_core/peer_core.hpp:21-26) is explicitly "the portable
  peer minus the platform": owns registry, live/pending graph pair (swap+migrate),
  param/edit queues, HTTP control surface, values snapshot, AudioRegion.
- The executor core is platform-free: signal_graph_tick.cpp, signal_graph_plan.cpp,
  parse_graph (signal_graph_serial.cpp) — pure C++, all I/O via node descriptors.
- Only ~5 `#ifdef` blocks in the whole tree (log.hpp 3-way; audio_engine.cpp Android
  mic ×2; ws_link.hpp Emscripten ×1); other platform splits are CMake target
  selection (audio_output_android.cpp vs audio_output_sdl.cpp; ws_link.cpp vs
  ws_link_em.cpp; XR suite only when CMAKE_CROSSCOMPILING, CMakeLists.txt:131-150).
- The audio region is the executor-node prototype: AudioEngine (singleton, owns
  device stream + block scheduler; components/audio_engine) + AudioRegion (owned by
  PeerCore, frame↔block boundary mappings; components/audio_region) + DacNode
  bridging in (components/dac_node).
- Boundary adapters designed and partially built: planning/edge_executor.design.md —
  regions (render/audio/worker/net), type lattice, auto-inserted mappings (z⁻¹,
  latch, snapshot, queue, ring, net). Region inference: signal_graph_plan.cpp:95-117
  (dac upstream closure through audio edges → Block; else Frame).
- Remote-peer machinery: PeerCore::connect_peer registers each advertised type as a
  proxy descriptor "type@host:port" (components/remote_node); WebSocket bridge
  (components/ws_link); mDNS (components/mdns_advertiser).
- Graphs as data precedents: graph JSON strings flow through queue_edit;
  components/graph_source reifies the enclosing graph as wired outputs (keys,
  positions, count) via a resource-holder context seam (set_context) — the same seam
  proposed for the platform-context stash.
- Subgraphs: components/subgraph_node + subgraph_descriptor (clone template, slot
  trampolines); registry loads *.json subgraph plugins from graphs_dir
  (component_registry.cpp:27-54) and .so plugins via dlopen
  (component_registry.cpp:69-99, hot-reload with retired_ list).
- Pull observability (endpoints v6 phase D): snapshot_values on demand
  (signal_graph_tick.cpp:362-370); HTTP /values, /palette.
- Lifting/conformability executor: planning/conformability_lifting.md,
  kanban/ready/conformability_executor.md (rung 3 done; GPU instancing pending) —
  the lifted_store keyed-identity migration is prior art for identity-preserving
  lowering.

### What violates the target (the native-outside-the-graph inventory)

| Shell | Native outside the graph | ~LOC |
|---|---|---|
| Quest (components/app: app.cpp 466 + xr.cpp 125) | android_main, JNI multicast lock (app.cpp:125-147), XR instance/system polling (xr.cpp:35-124), XrSessionObj (components/xr_session), FrameLoop (components/frame_loop — owns xrWaitFrame/Begin/End and *drives the tick* via callback), Renderer + EglContext + EyeSwapchain (components/renderer, egl_context, eye_swapchain), Input + pump_xr_sources() (app.cpp:247-310 — pushes poses into head_pose/controller nodes from outside), Scene (components/scene, legacy immediate-mode residue), mdns start (app.cpp:349), hand registry (app.cpp:149-244) | ~990 |
| Host (app/spectator/spectator_main.cpp 80 + components/host_app, host_gl_window, host_gl_context) | GLFW window / headless EGL+FBO, fps input polling, main loop drives frame(), hand registry (host_app.cpp:67-179) | ~265 |
| Web (app/web/web_main.cpp 291, separate CMake tree) | WebGL2 context, DOM input, emscripten_set_main_loop + 250ms network pump, own kDefaultGraph, own inline registry | ~291 |

Cross-cutting: three divergent registration lists; shells drive the executor
(begin_frame → pump_contexts → tick → collect_edits from platform loops); graph
selection hardcoded (editor_default_graph.hpp kEditorGraph embedded, passed as
Config.default_graph_json from host_app.cpp:183 and app.cpp:337); HTTP owned by
PeerCore (peer_core.hpp:79), unauthenticated.

app/hello is a trivial build-test stub; ignore.

### Existing docs the planning session should have open

- `planning/vision.md` — the guiding star: "any time we must restart the app to
  change or extend it, we are failing a crucial test." The whole architecture serves
  this; the engine-graph model extends it to the compiler layer.
- `planning/bootloader.md` — the converged design (this session's main artifact).
- `planning/datasets.md` — store design (this session).
- `planning/edge_executor.design.md` — regions, type lattice, boundary mappings,
  legal connections. The adapter vocabulary the executor packages will use.
- `components/peer_core/peer_core.design.md` — "the core that spins up and executes
  the graph that spins up and executes the graphs… recursively to the leaf nodes"
  (Travis, 2026-06-12): the recursion the tower formalizes; shells own registry
  population; embedded default graph rationale.
- `kanban/backlog/freezer.md` — frozen programs: codegen to portable C++, target
  tiers, **provenance doctrine** (artifacts carry source JSON for unfreezing);
  postponed pending v6/spans; freezing bypasses the bootloader ("non-frozen" scope).
- `planning/conformability_lifting.md`, `kanban/ready/conformability_executor.md` —
  lifting strategies; keyed lifted_store migration (prior art for identity mapping).
- `planning/render_as_nodes.md`, `planning/status.md` — render region history,
  current state (DrawFn retired, editor decomposed).
- `adr.md` — where architectural evolution is supposed to be recorded; this session's
  outcome should probably be distilled into an ADR entry once ratified.

### Precedents cited in discussion (for the record)

Reflective towers / procedural reflection (grounding + laziness discipline); Nix
derivations (content addressing, memoization, substituters, GC roots — resonant since
the project standardizes on Nix); build-systems-à-la-carte; spreadsheets (data +
derivation in one editable surface with automatic re-derivation — what the editor
becomes); git object model (immutable objects + refs; reflog = undo history, cf.
components/undo_node); Erlang-style supervision (spawn + restart policy).

## Suggested session structure

1. Ratify or amend the settled-decisions list above (each is one line to overturn now,
   a refactor to overturn later).
2. Take the open questions in order 1→3 (data model, lowering identity, extension
   points) — these gate implementation; 4→6 (trust, formats, retention) are
   store-scoped; 7→8 are engineering discipline.
3. Specify the dataset payload + provenance header (open Q5 collapses into this).
4. Design the `lower` node's interface (slice 2) precisely enough to write its test.
5. Cut kanban items for slices 1–4 and distill ratified decisions into adr.md.
