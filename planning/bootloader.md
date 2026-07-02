# Graph executor bootstrapping — design draft

Status: design draft, 2026-07-02, revised after alignment discussion. Converged: the
three-level model below. Open: spawn vs exec (Q3). Supersedes the 2026-07-01
assessment (appendix).

## The three-level model

A self-hosting compiler pipeline:

- **Application graph** — authored content, platform- and execution-agnostic. No
  executors, regions, or adapters. What the editor edits and peers exchange.
- **Engine graph** — a graph, run by the engine, that examines application graphs and
  constructs execution graphs: infers regions (audio, xr, gl, remote-peer…), inserts
  the executors appropriate to *this* environment, materializes boundary adapters
  (edge_executor.design.md: latch, snapshot, queue, ring, net). The engine graph is
  the portable cross-platform story — the same everywhere; platform difference
  appears in what it emits, not what it is.
- **Execution graph** — the lowered artifact: explicit executor containment, platform
  nodes, adapters. Never authored by hand.

Resolved by this model:
- Containment vs inference (old Q1) was a false dichotomy: inference is the app-graph
  representation, containment the execution-graph representation; lowering maps them.
- Platform selection: app graphs stay portable; execution graphs are where
  platform-specific nodes appear, placed by the engine graph's environment
  observation.

## Executor nodes

A node owning (a) a subgraph and (b) the execution context that ticks it — thread,
subprocess (boundary edges degrade to net mappings), async event loop.

**Pacing: the executor owns it** (resolved Q2), along with the platform resource.
Inner node families bind through a per-flavor **contract**: audio executor ↔ dac/adc;
xr executor ↔ head_pose/controllers/layer submission; gl executor ↔ draw nodes. This
matches the existing code (AudioRegion/AudioEngine own thread, device, pacing;
DacNode bridges in) and retires the three hand-rolled contracts the survey found:
the RenderRegion singleton, pump_xr_sources(), dac's AudioEngine::instance() reach.
Lifecycle: instantiate → acquire context → run → stop, plus restart policy.

## Stage 0 — the native kernel

Identical logic on every platform. A *naive* executor: flat graph, no regions, no
pacing, free-running — exactly enough to run the engine graph (control-rate, no
realtime constraints). Contains:

1. Naive tick core: parse, plan (flat), tick, migrate.
2. Registry, populated by target-supplied `syg_register_linked(registry)`
   (CMake-generated TU per target; loud link failure; readable capability manifest).
   Runtime inspection of linked nodes = reading ComponentRegistry's map (already the
   palette).
3. Graph-as-value payload type + primitive query/transform node implementations —
   the one substantial new port-system addition. (Precedent: graph JSON strings flow
   through queue_edit; graph_source reifies the enclosing graph as wired data.)
4. Executor-node primitives implemented in C++ *as nodes*: thread spawn, adapters,
   contracts, and an instantiate/swap-with-migration node.
5. Embedded boot graph via a second target-supplied symbol (may differ per target;
   stage-0 code never branches on platform).
6. Opaque platform-context stash (trampoline hands over android_app* etc.; only
   platform nodes read it). Trampolines <10 lines: while-loop (desktop),
   emscripten_set_main_loop (web), android_main (Quest).

Engine *logic* — which passes, in what order, the region rules — is graph content:
live-editable, satisfying the no-restart criterion (planning/vision.md) at the
compiler layer.

## Boot sequence

Stage 0 ticks the boot graph → boot graph spawns the engine graph → engine graph
receives application graphs from instruction sources (cli_args from the stash,
http_server, ws/peer links, hardcoded constant), lowers them, instantiates execution
graphs. Edits arrive against the app graph; the engine graph re-lowers; swap+migrate
carries state.

Q3 (open, recommendation: spawn): the boot graph stays resident supervising the
engine graph with a restart policy — the engine graph is critical infrastructure a
bad edit could wedge; the boot graph is the recovery story. Matches "the core that
spins up and executes the graph that spins up and executes the graphs… recursively"
(peer_core.design.md).

PeerCore dissolves: registry → stage 0; swap/migrate/queues → executor machinery;
HTTP routes, mdns, values/probe, screenshots → nodes.

## Hard problems

1. **Identity-preserving lowering.** State lives in execution-graph nodes but must
   survive re-lowering keyed by app-graph identity: lowering must be deterministic
   and emit an app-node-id → execution-node-id map for swap+migrate. Main
   engineering risk.
2. **Engine-graph debuggability.** A graph transforming graphs needs purpose-built
   probes early; pull-observability is the substrate.
3. GL/XR nodes must not acquire resources in create() (first-tick lazy init; fail
   loud if ticked without context) — containment handles ordering, not allocation
   discipline.

## Migration path (strangler pattern)

1. Generated per-target registration TU (kills the three hand lists: 106/92/41).
2. Wrap the existing C++ build_plan pipeline as a single `lower` node; initial engine
   graph = receive app graph → lower → instantiate. Semantically the current system,
   restructured into the three-level shape.
3. Retrofit AudioRegion as the first executor node with the dac/adc contract — the
   pattern already exists there in disguise; no XR risk.
4. Stage-0 extraction from PeerCore + trampolines; host first.
5. Quest platform executor: xr executor absorbing xr.cpp + XrSessionObj + FrameLoop +
   Renderer/EglContext/EyeSwapchain; input via the xr contract (delete
   pump_xr_sources); retire Scene; mdns/http as nodes.
6. Web onto the same bootloader.
7. Factor passes out of the monolithic `lower` node into engine-graph vocabulary one
   at a time (region inference, adapter insertion, executor placement) — the "second
   step: properly factoring the nodes," applied to the compiler itself.

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

- Pacer seam (old D1) → executor nodes; executor owns pacing + resource, inner nodes
  bind via contract.
- Entry symbols (old D2) → accepted constraint; trampolines.
- Registration/probing (old D3/D4) → generated TU stands; per-platform boot graphs
  acceptable; the invariant is stage-0 platform independence. Superseded in part by
  the engine-graph model: portability lives in the engine graph.
- Resource ordering (old D5) → containment orders execution; allocation discipline
  (no resources in create()) remains.
- Frozen programs unaffected: freezing (kanban/backlog/freezer.md) bypasses the
  bootloader by design.
