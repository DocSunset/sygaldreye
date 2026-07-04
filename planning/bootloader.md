# Graph executor bootstrapping — design draft

Status: design draft, 2026-07-02, fourth revision. Converged: roles model, executor
packages, spawn+exec both. Provenance/detachment RESOLVED (see "Every level
editable" below); naming in planning/naming.md; store in planning/datasets.md;
trust in planning/trust.md. Original assessment in appendix.

## The general principle: datasets and derivations

The graph is not the fundamental object. A **dataset** is; a **pipeline** is a
derivation from datasets to datasets; **provenance-or-fork** is the law relating
derived data to its sources. A graph is a dataset an executor can run. Compilation
is a derivation; freezing is a derivation whose output kind is C++; an edit is a
pass or a fork. The same principle carries an additive synthesizer (partials
dataset → oscillator pipeline), atomic decomposition analysis-resynthesis, a video
editor-compositor, projection mapping, and an ML workbench (dataset → training
derivation → model artifact with provenance to its training set) — new dataset
kinds and vocabulary, no new principles.

Consequences taken now:
- The graph-as-value payload (stage 0, item 3) is conceived as the general
  **dataset payload**: kind + content + provenance reference. Graph is the first
  kind.
- Provenance records are content-hash-friendly from day one, so derivation
  memoization (Nix-style; the project already standardizes on Nix) is possible
  later without retrofit.
- Pipelines run in two modes with existing executors: **streaming** (live
  frame/block tick) and **derivation** (run-to-completion with provenance
  recording — a worker-region execution; the worker region is the build system).
- The five applications above are validation targets, not requirements: design the
  primitive so it doesn't preclude them; build only the graph case now.

Where datasets live, who sees them, and how: planning/datasets.md.

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

## Every level editable: provenance with detachment (RESOLVED 2026-07-02)

All graphs are runtime-editable, including realized artifacts. Every realized graph
carries **provenance** to its definition (freezer doctrine). The round-trip
semantic, final form, one breath: **realized views are editing surfaces that write
back through the route map; compilation inserts defaults only where absent;
conditional behavior is a pass; refusing write-back is a fork.**

- A mapping is just a node; an app graph may contain e.g. a custom smoother
  explicitly, and compilation inserts its default boundary mapping **only where no
  mapping already sits** (the precise meaning of edge_executor's "replaceable by
  the user").
- Editing the *realized view* is projection editing (the spreadsheet principle):
  the editor translates the edit through the compilation's inverse route map into an
  app-graph edit. No override/patch dataset — that would be a second place where
  definition lives, and it would drift.
- Genuinely conditional-on-compilation desires ("only when this boundary lands
  cross-thread") are the definition of a **pass** — usually a tiny one, spliced
  like any capability-package pass. Promotion to a pass is always a deliberate
  authoring act, never automatic.
- An edit the app-level vocabulary can't express is a vocabulary gap (guiding
  star: fix upstream), not a case for shadow state.
- **Fork** = the ref stops tracking the derivation's output — an ordinary,
  recorded rebind (naming.md). Degenerate case: a graph with no upstream
  definition is an app graph whose compilation is identity.
- Param/default writes are NOT this problem: they edit the persisted defaults
  node (datasets.md "composite node"), no compilation involved.

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

A package splice is a derivation over the engine graph (graph → graph) — same
primitive again, driven by environment observation. But raw transformers don't
compose (pass-ordering hell), so: **extension points aren't machinery — they're
ports.** The initial engine graph *publishes inlets* (fan-ins: recognize-region,
construct-context, choose-adapters); the well-behaved package **wires into them**
rather than rewriting. Full transformer power stays available for surgery — it's
just an edit — but then the package owns the breakage. **Order is wiring** (the
draw-chain precedent: topological order is submission order); pass order is
visible patch structure, which is also free debuggability. "Name the initial
ports" is migration-slice-5 interface design, no longer an architecture question.

**Capability-driven placement**: when a package is absent locally, realization can
place that region on a remote peer advertising the capability — net adapters and
remote-peer machinery already exist. Quest's worker region is compiled onto the
Linux peer; a browser peer's XR region compiles onto a remote peer and the browser
becomes a spectator. Cross-network placement is a fallthrough case of compilation,
not a feature.

## Stage 0 — the native kernel

REFRAMED 2026-07-03: stage 0 is **a frozen realization of the boot graph** — not
special machinery but ordinary (native) nodes whose parsing, planning, and
instantiation happened at *build time* instead of runtime. The one irreducible
property is pre-existence: something must already be running before anything can
be derived at runtime. Parse, plan, swap, resolve, even the tick loop are all
nodes; what cannot be decomposed is that this one graph arrives already derived.
Per the freezer doctrine it carries provenance for unfreezing (boot graph JSON +
native manifest), and its build-time derivation is a Nix derivation — provenance
continues below stage 0 into the flake lock; fiat retreats to the
toolchain/physics boundary.

Identical logic on every platform; vanishingly minimal. A naive executor — flat
graph, no regions, no pacing, free-running — exactly enough to run the first engine
graph (control-rate). Contains (read as the frozen graph's capability manifest):

1. Naive tick core: parse, plan (flat), tick, migrate.
2. Registry via target-supplied `syg_register_linked(registry)` (CMake-generated TU;
   loud link failure; readable capability manifest). Linked-node inspection =
   reading ComponentRegistry's map (already the palette).
3. Dataset payload (kind + content + provenance reference; graph is the first
   kind) + primitive query/transform nodes + the **naive resolver** (hash → bytes
   in the local object directory — grounds resolution-as-graphs the way the naive
   executor grounds the tower; stays independently invokable forever). (Precedent:
   graph JSON through queue_edit; graph_source.)
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
embedded constant), compiles, realizes. Edits arrive at any level; provenance decides
propagation; slot-swap+migrate carries state.

PeerCore dissolves: registry → stage 0; swap/migrate/queues → slot machinery; HTTP
routes, mdns, values/probe, screenshots → nodes.

## Hard problems

1. **Identity-preserving compilation**: deterministic, emitting route → route maps
   so state survives re-compilation (= compilation emits stable local names;
   naming.md).
   Main engineering risk — and it now also carries projection editing (write-back
   through the inverse map) and defaults rebasing.
2. ~~Provenance/detachment semantics~~ RESOLVED 2026-07-02: naming.md is the data
   model; "Every level editable" above is the editing rule.
3. **Engine-graph debuggability**: purpose-built probes early; pull-observability is
   the substrate. Stakes raised by self-hosted resolution/repair — the naive
   resolver stays independently invokable as insurance.
4. Allocation discipline stands: no resource acquisition in create(); first-tick
   lazy init; fail loud without context.

## Migration path (strangler pattern — unchanged by the vision, by design)

1. Generated per-target registration TU (kills the three hand lists: 106/92/41).
2. Wrap existing C++ build_plan as a single `compile` node; initial engine graph =
   receive → compile → realize. Semantically the current system in the new shape.
3. Retrofit AudioRegion as the first capability package (vocabulary: dac/adc;
   passes: today's inference; machinery: existing thread/device code). No XR risk.
4. Stage-0 extraction from PeerCore + trampolines; host first.
5. Declare engine pipeline extension points (prerequisite for a second package).
6. Quest package: xr executor absorbing xr.cpp + XrSessionObj + FrameLoop +
   Renderer/EglContext/EyeSwapchain; input via the xr contract (delete
   pump_xr_sources); retire Scene; mdns/http as nodes.
7. Web onto the same bootloader.
8. Factor passes out of the `compile` monolith into engine-graph vocabulary; decide
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
  containment the execution-graph view; compilation maps them.
- Resource ordering → containment orders execution; allocation discipline remains.
- Spawn vs exec → both, one primitive; stage 0 always spawn+resident.
- Frozen programs: freezing is the extreme point of the same spectrum — realization
  to C++ with provenance for unfreezing — and bypasses the bootloader by design.
