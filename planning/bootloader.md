# Graph executor bootstrapping — design draft

Status: design draft, 2026-07-02, third revision after alignment. Converged: roles
model, executor packages, spawn+exec both. Open: provenance/detachment details.
Original assessment in appendix.

## Roles, not types

App / engine / execution are roles in a compilation relationship, not disjoint kinds.
Every graph is **defined** (as data — in memory or JSON), **compiled** by an engine
graph with matching capabilities, and **realized** as an execution graph running on
hardware across a network of peers, devices, processes, threads, event loops. Engine
graphs are themselves defined/compiled/realized: a reflective tower.

- The regress grounds at stage 0: the naive C++ executor is the fixed base realizing
  the first engine graph.
- **Laziness rule**: the tower is potential, not resident. A level's engine graph is
  instantiated only when someone edits at that level. Never N meta-levels running.

## Every level editable: provenance with detachment

All graphs are runtime-editable, including realized artifacts. The round-trip
semantic (an execution graph is edited, then its app graph re-lowers — who wins?):

- Every realized graph carries **provenance** to its definition (doctrine already
  established by the freezer: frozen artifacts carry source JSON for unfreezing).
- An edit to a realized graph either becomes a **pass** appended to the pipeline that
  produced it (editing the compiler; persists across re-lowering), or **forks
  provenance**: the editor now owns the graph at that level, detachment is recorded,
  upstream re-lowering no longer applies. Tracked, never silent.
- Degenerate case: a graph with no upstream definition is an app graph whose
  compilation is identity. Uniform editability falls out.

## Spawn and exec: one primitive, two call sites

The general primitive is "replace the subgraph of slot X, migrating state."
**Exec** = swap your own slot (reduced footprint, no fallback). **Spawn** =
instantiate a child slot and park (resident fallback + restart policy). Per-stage
policy bit; both supported. Stage 0 spawns stage 1 and remains resident — its cost
is a parked loop and the embedded boot graph, and it is the one tower level that
cannot be edited away.

## Executor regions are capability packages

An executor flavor (audio, xr, gl, worker, subprocess, remote-peer) is a world
apart — its own app/engine/execution triple — spliced into the main engine at
runtime when its capability is available. Three parts:

1. **Vocabulary**: its platform nodes (dac/adc, xr_session, draw…), present only
   where linked.
2. **Passes**: extensions to the engine's realization pipeline — recognize its
   region in an app graph (dac-closure inference, generalized), construct its
   execution subgraph and context, choose boundary adapters (latch, snapshot, queue,
   ring, net). The dac/adc "special contract" is just this package's rewrite rule
   binding those nodes to its context.
3. **Machinery**: C++ context internals (thread, device callback, frame envelope)
   inside its nodes. Executor owns pacing and the platform resource.

Requires the engine graph to be a **pipeline with declared extension points**;
splicing a package in is itself an edit to the engine graph, driven by environment
observation. Same primitive again.

**Capability-driven placement**: when a package is absent locally, realization can
place that region on a remote peer advertising the capability — net adapters and
remote-peer machinery already exist. Quest lowers its worker region onto the Linux
peer; a browser peer lowers XR remotely and becomes a spectator. Cross-network
placement is a fallthrough case of lowering, not a feature.

## Stage 0 — the native kernel

Identical logic on every platform; vanishingly minimal. A naive executor — flat
graph, no regions, no pacing, free-running — exactly enough to run the first engine
graph (control-rate). Contains:

1. Naive tick core: parse, plan (flat), tick, migrate.
2. Registry via target-supplied `syg_register_linked(registry)` (CMake-generated TU;
   loud link failure; readable capability manifest). Linked-node inspection =
   reading ComponentRegistry's map (already the palette).
3. Graph-as-value payload + primitive query/transform nodes — the substantial new
   port-system addition. (Precedent: graph JSON through queue_edit; graph_source.)
4. Executor primitives implemented in C++ *as nodes*: thread spawn, adapters,
   contexts, and the slot-swap-with-migration node.
5. Embedded boot graph via a second target-supplied symbol (may differ per target;
   stage-0 code never branches on platform).
6. Opaque platform-context stash (android_app* etc.; only platform nodes read it).
   Trampolines <10 lines: while-loop / emscripten_set_main_loop / android_main.

Not in stage 0: HTTP, files, GL/XR/audio, selection logic, region rules.

## Boot sequence

Stage 0 ticks the boot graph → spawns the engine graph and parks as fallback →
engine graph observes environment, splices available capability packages, receives
app graphs from instruction sources (cli_args stash, http_server, ws/peer links,
embedded constant), lowers, realizes. Edits arrive at any level; provenance decides
propagation; slot-swap+migrate carries state.

PeerCore dissolves: registry → stage 0; swap/migrate/queues → slot machinery; HTTP
routes, mdns, values/probe, screenshots → nodes.

## Hard problems

1. **Identity-preserving lowering**: deterministic, emitting app-node-id →
   execution-node-id maps so state survives re-lowering. Main engineering risk.
2. **Provenance/detachment semantics**: must be decided before execution graphs
   become editable.
3. **Engine-graph debuggability**: purpose-built probes early; pull-observability is
   the substrate.
4. Allocation discipline stands: no resource acquisition in create(); first-tick
   lazy init; fail loud without context.

## Migration path (strangler pattern — unchanged by the vision, by design)

1. Generated per-target registration TU (kills the three hand lists: 106/92/41).
2. Wrap existing C++ build_plan as a single `lower` node; initial engine graph =
   receive → lower → realize. Semantically the current system in the new shape.
3. Retrofit AudioRegion as the first capability package (vocabulary: dac/adc;
   passes: today's inference; machinery: existing thread/device code). No XR risk.
4. Stage-0 extraction from PeerCore + trampolines; host first.
5. Declare engine pipeline extension points (prerequisite for a second package).
6. Quest package: xr executor absorbing xr.cpp + XrSessionObj + FrameLoop +
   Renderer/EglContext/EyeSwapchain; input via the xr contract (delete
   pump_xr_sources); retire Scene; mdns/http as nodes.
7. Web onto the same bootloader.
8. Factor passes out of the `lower` monolith into engine-graph vocabulary; decide
   provenance/detachment before opening execution graphs to the editor.

---

# Appendix: original assessment (2026-07-01)

## Where we were

`PeerCore` already is "the portable peer minus the platform" (peer_core.hpp:21-26);
`signal_graph`, `component_registry`, and `parse_graph` have no platform dependencies;
platform audio/net differences already live in per-target sources selected by CMake
(5 `#ifdef` blocks in the whole tree); the block region already demonstrates the key
pattern — the audio region owns the device and paces its nodes.

Violations of the target:

| Shell | Native outside the graph | LOC (approx) |
|---|---|---|
| Quest (`components/app`) | `android_main`, XR instance/system polling (xr.cpp), `XrSessionObj`, `FrameLoop`, `Renderer`+`EglContext`+`EyeSwapchain`, `Input` + `pump_xr_sources()`, `Scene`, mdns start, JNI multicast lock, hand-listed registry (92 `register_builtin`) | ~990 |
| Host (`app/spectator`, `host_app`) | GLFW/EGL window or headless FBO, fps input polling, main loop, hand-listed registry (106) | ~265 |
| Web (`app/web`) | WebGL context, DOM input, rAF loop + 250ms net pump, its own default graph, its own inline registry (41) | ~291 |

Cross-cutting: three divergent hand-maintained registration lists; the shell drives
the executor (`begin_frame → pump_contexts → tick → collect_edits` from platform
loops); graph selection hardcoded (`kEditorGraph`); host services (HTTP, mdns,
plugin scan, XR pose pumping) wired outside the graph.

## Resolution notes from discussion

- Pacer seam → executor packages own pacing + resource; node families bind via the
  package's rewrite rules. Retires the RenderRegion singleton, pump_xr_sources(),
  and dac's AudioEngine::instance() reach.
- Entry symbols → accepted constraint; trampolines.
- Registration/probing → generated TU stands; per-platform boot graphs acceptable;
  portability lives in the engine graph.
- Containment vs inference → false dichotomy: inference is the app-graph view,
  containment the execution-graph view; lowering maps them.
- Resource ordering → containment orders execution; allocation discipline remains.
- Spawn vs exec → both, one primitive; stage 0 always spawn+resident.
- Frozen programs: freezing is the extreme point of the same spectrum — realization
  to C++ with provenance for unfreezing — and bypasses the bootloader by design.
