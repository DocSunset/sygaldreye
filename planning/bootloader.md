# Graph executor bootstrapping — design draft

Status: design draft for alignment, 2026-07-02. Supersedes the 2026-07-01 assessment
(preserved below). Ratified direction from discussion: generalize the audio-region
pattern into executor nodes; stage 0 is platform-independent code, but boot *graphs*
and registries may be platform-specific — the target is that all platform-specific
code lives in nodes, not that all platforms run the same graphs.

## Executor nodes

A node that owns (a) a subgraph and (b) an execution context in which that subgraph's
plan ticks. To its parent it is an ordinary node; edges crossing its boundary get the
documented mapping adapters (edge_executor.design.md: latch, snapshot, queue, ring,
net) chosen by payload and cadence.

- Flavors are contexts: `thread_executor`, `subprocess_executor` (boundary edges
  degrade to net mappings — remote-peer machinery pointed at a child process),
  `async_executor` (event loop; async-function sub-subgraphs).
- Lifecycle: instantiate → acquire context → run → stop, plus restart policy
  (supervision falls out).
- Pacing (proposed, OPEN Q2): executors are generic; a resource node *inside* the
  subgraph claims pacing — dac claims audio callbacks (the existing precedent),
  xr_session claims xrWaitFrame, a window node claims vsync. No pacer → free-run at a
  rate param. The pace-claim interface between executor and subgraph node is the one
  new seam.
- Regions become containment (OPEN Q1): explicit membership in an executor's subgraph
  replaces dac-upstream-closure inference. Inference remains only as a migration
  bridge; the editor gains "place node inside executor."

AudioRegion is the hardcoded prototype: thread executor + dac pacer. Retrofit it first.

## Stage 0 — the native kernel

Identical logic on every platform; contains exactly:

1. Executor core: parse, plan, tick, migrate, boundary adapters (already portable).
2. Registry, populated by one target-supplied symbol `syg_register_linked(registry)`
   (CMake-generated TU per target — loud link failure, readable capability manifest).
   Runtime inspection of linked nodes = reading ComponentRegistry's map; already
   surfaced as the palette (`sorted_types_`, /palette).
3. Embedded boot graph via a second target-supplied symbol. May differ per target;
   stage-0 code never branches on platform.
4. Opaque platform-context stash (trampoline hands over android_app* etc.; only
   platform nodes read it — graph_source::set_context pattern).
5. Root context: `init()` + `step()`. Trampolines (<10 lines each) choose the driving
   shape: while-loop (desktop), emscripten_set_main_loop (web), android_main (Quest).

Not in stage 0: HTTP, files, GL/XR/audio, graph selection logic.

## Boot sequence

Boot graph (embedded, per-platform) contains one executor node whose subgraph is the
platform layer: context/pacer nodes (egl + xr_session / gl_window / raf), input source
nodes, `http_server`, `mdns`, instruction sources (`cli_args` from the stash, ws/peer
links, hardcoded constant). The platform graph holds the application graph in a
swappable slot.

The edit primitive generalizes PeerCore's swap+migrate from "replace the graph" to
"replace the subgraph of node X, migrating state" — serving boot handoff, live edit,
and replacement alike.

Transfer is spawn, not exec (proposed, OPEN Q3): the boot graph stays resident as a
near-idle supervisor holding the executor node and its restart policy. This is the
watchdog against a platform/app graph editing away its own control surface, and
matches "the core that spins up and executes the graph that spins up and executes the
graphs… recursively" (peer_core.design.md).

PeerCore dissolves: registry → stage 0; swap/migrate/queues → executor machinery;
HTTP routes, mdns, values/probe, screenshots → nodes.

## Open questions

1. Commit to containment-based regions, deprecating dac-closure inference? (editor UX
   + plan builder consequences; avoid maintaining two region theories)
2. Pacer-inside-generic-executor (dac precedent) vs platform-flavored executors (if
   the XR frame envelope is different in kind from audio callbacks).
3. Spawn vs exec for stage transfer — boot graph as durable supervisor?

## First slices (once aligned)

1. Generated per-target registration TU (kills the three hand lists: 106/92/41).
2. Retrofit AudioRegion as `thread_executor` + pacer-claiming `dac` — proves the
   abstraction where it already exists in disguise, no XR risk.
3. Stage-0 extraction from PeerCore + trampolines; host first.
4. Quest platform graph: xr_session pacer node absorbing xr.cpp + XrSessionObj +
   FrameLoop + Renderer/EglContext/EyeSwapchain; input polling into source nodes
   (delete pump_xr_sources); retire Scene; mdns/http as nodes.
5. Web onto the same bootloader; subgraph-slot swap as the general edit primitive.

---

# Appendix: original assessment (2026-07-01)

## Where we were

`PeerCore` already is "the portable peer minus the platform" (peer_core.hpp:21-26);
`signal_graph`, `component_registry`, and `parse_graph` have no platform dependencies;
platform audio/net differences already live in per-target sources selected by CMake
(5 `#ifdef` blocks in the whole tree); the block region already demonstrates the key
pattern — the `dac` node owns the audio device and paces its region.

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

- Original D1 (pacer seam) → subsumed by executor nodes, above.
- Original D2 (entry symbols) → accepted as constraint; trampolines.
- Original D3/D4 (registration = linkage, capability probing) → generated TU stands;
  per-platform boot graphs are acceptable, so registry probing is a convenience, not
  the selection mechanism. Stage-0 platform independence is the invariant.
- Original D5 (resource ordering) → resolved by containment: a subgraph cannot tick
  before its executor's context exists. GL nodes still must not acquire resources in
  `create()` (first-tick lazy init; fail loud if ticked with no context).
- Frozen programs unaffected: freezing (kanban/backlog/freezer.md) bypasses the
  bootloader by design.
