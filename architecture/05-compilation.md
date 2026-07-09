# Chapter 5 — Compilation, the tower, projection editing (CMP)

*A builder implementing this chapter delivers: the engine graph (initially one
`compile` node wrapping today's build_plan), the compilation map, published
extension ports, projection editing, and the tower's laziness discipline.*

## Design

**Roles, not types.** Every graph is defined (data), compiled (by an engine
graph whose capabilities match), realized (instantiated on hardware across
peers/processes/threads). Engine graphs are themselves
defined/compiled/realized — a reflective tower grounded at stage 0, **lazy**:
a level's engine graph is instantiated only when someone edits at that level;
never N meta-levels resident.

**Compilation is a committed derivation** (inputs: app graph hash + engine graph hash  to 
execution graph dataset), so it memoizes like any committed derivation. It must be
**deterministic** and emit the **compilation map** (app route to execution
route) — the enabler of state migration across re-compilation and of
projection editing. Deterministic = compilation assigns stable local names
(the lift machinery's "key by cell value, else index," stated generally).

**Extension points are ports.** The initial engine graph publishes fan-ins —
recognize-region, construct-context, choose-adapters (more may be surfaced as
the `compile` monolith is factored) — and capability packages *wire into
them*. Raw graph to graph surgery remains possible (a splice is an ordinary
edit) but the package then owns the breakage. **Order is wiring**: pass order
is visible patch structure (the draw-chain precedent), which is also the
debuggability story.

**Projection editing (detachment, resolved).** Realized views are editing
surfaces: an edit to an execution graph translates through the inverse
compilation map into an app-graph edit. Compilation inserts default boundary
mappings only where no mapping already sits. Conditional-on-compilation
behavior ("only when this boundary lands cross-thread") is a deliberate
**pass**. Refusing write-back is a **fork**: the ref stops tracking, recorded.
There is NO override/patch shadow dataset — definition lives in one place.
An edit inexpressible in app vocabulary is a vocabulary gap: fix upstream
(the guiding star applied to the compiler).

**Engine-graph debuggability** is a first-class concern: purpose-built probes
early; pull-observability is the substrate; pass order readable off the patch.

## Requirements

**cmp.compile_is_derivation** compile(app, engine) to execution dataset
with recipe provenance; memoized on input hashes.
- cmp.compile_is_derivation.memoized: compiling hello-cosine twice runs passes once (counter).
- cmp.compile_is_derivation.defaults_edit_skips_structural: editing the *defaults* node only does not re-run structural passes
  (topology hash unchanged to memo hit on the structural stage).

**cmp.determinism_and_map** Same inputs to identical execution graph hash
AND identical compilation map.
- cmp.determinism_and_map.deterministic_hash: compile hello-cosine a hundred times; one distinct output hash.
- cmp.determinism_and_map.map_covers_instances: the map contains `nodes/osc0 to block/osc0` (or equivalent stable
  scheme) and every app instance appears exactly once.

**cmp.extension_ports** Packages integrate exclusively by wiring into
published fan-ins in the well-behaved path; the audio package passes this
bar. (Strengthened by ADR-034: both criteria observe the realized engine
plan, never the compiler's self-report.)
- cmp.extension_ports.additive_splice: the audio splice lands as ordinary edit ops on the live engine
  graph; the diff against vanilla is additive wiring only (no rewrites of
  existing engine nodes).
- cmp.extension_ports.order_is_topological: pass execution order equals topological order of the engine
  patch, observed as per-instance tick counts in the realized engine plan
  (never a list the compiler prints about itself).

**cmp.projection_editing** Edits in realized views write back through
the inverse map; insert-if-absent for default mappings.
- cmp.projection_editing.writeback_smoother: replace hello-cosine's latch with `smoother` in the execution
  view to the app graph now contains smoother0 wired at edge 1; re-compiling
  inserts NO latch there.
- cmp.projection_editing.vanished_route_conflict: an edit whose target route vanished upstream surfaces a conflict
  (not silence, not a crash).

**cmp.fork** Rebinding a ref away from a derivation's output records
detachment; upstream re-compilation no longer applies; the fork is visible in
lineage queries.
- cmp.fork.fork_untouched: fork hello-cosine's execution graph, edit upstream app graph,
  re-compile: the forked ref is untouched and lineage marks the detachment.

**cmp.laziness** No engine graph above the level being edited is
resident.
- cmp.laziness.zero_resident_engine: steady-state hello-cosine playback instantiates zero engine
  instances (only stage 0's parked loop + the running execution graph);
  opening the engine editor instantiates exactly one level.

**cmp.identity_across_recompilation** State survives app-graph edits
end-to-end (composition of cmp.determinism_and_map's map with exe.migration's migration).
- cmp.identity_across_recompilation.phase_continuous: while sounding, add noise0 to hello-cosine, re-compile, swap:
  osc0's phase continuous; latch state preserved.

**cmp.retired** The strangler migration path assumed
refactoring the probe; the ratified greenfield build (appendix,
18-appendix-greenfield.md) supersedes it. The probe is reference and
salvage, never migrated. Kept as a numbered tombstone so citations resolve.

**cmp.engine_is_realized** The engine graph runs through
the one node contract: its passes are registered node types, instantiated
from the registry into a plan, ticked by the same crown/plan machinery as
any graph; compilation is a derivation-mode run of that plan (exe.derivation_mode). No
pass behavior executes outside a node hook; a bespoke evaluator over engine
data is scaffolding and names this requirement as its dissolution.
- cmp.engine_is_realized.engine_plan_run: compiling hello-cosine is observably the engine plan's run:
  per-pass-instance tick counters match the engine patch's topology, and an
  instrumented audit shows zero compile work outside node hooks (the
  hollow-engine regression).
- cmp.engine_is_realized.pass_edit_changes_compile: an ordinary edit op wiring an additional pass node into a
  published fan-in changes the next compile's output accordingly — no C++
  change, no restart; removing it restores the prior output hash.
- cmp.engine_is_realized.graph_pass_runs: a pass authored as a graph dataset (no C++) wired into a fan-in
  runs in the engine plan and is indistinguishable from a native pass
  (aut.four_routes extended to engine vocabulary).
- cmp.engine_is_realized.honest_lock: the lock is honest (ADR-026): the generator's descriptors commit
  through the canonical encoder as type nodes; graph locks carry those CIDs
  (never placeholder strings); resolver traversal (nam.addresses.location_independent's walk through
  lock → type → ports) and the runtime registry answer a port's promises
  from the SAME committed declaration — one representation, two caches.

## Freezing (FRZ)

Freezing is a committed derivation whose output kind is C++ — ratified (ADR-014): a
**backend of realization**. The engine pipeline ends in realize with two
backends: *interpret* (instantiate natives + plan) and *codegen* (emit a
fused, portable, typed C++ class). Same passes, same compilation map, same
provenance shape — one compilation model, never two. Ratified substance
(2026-06-12 card + 2026-07-03 amendments):

- **Codegen, not constexpr**: the generator emits plain typed C++ (identical
  optimizer visibility, sane compile times, readable errors, no compile-time
  registry fighting the plugin system).
- **Pure optimizer (ADR-013)**: per-sample islands are the interpreter's
  semantics too; freezing fuses them to loop-carried variables — cost
  changes, sound never does.
- **Portable-component-first**: the primary artifact is a plain C++ class —
  POD endpoint structs, simple tick(), dependency closure = kernel headers
  only; packagings are (a) plugin .so / WASM side module, (b) firmware
  scaffold, (c) Sygaldry-component adapter. Freeze-time baking: channel
  counts, rates, block size, subgraph composition become static.
- **Tiers are computed** from the dependency closure of natives used
  (a provenance query, surfaced in the editor): tier 1 freestanding (MCUs,
  anywhere C++ runs), tier 2 platform-lib (GLES/audio hosts), tier 3
  host-bound. A tier-1-plus-net artifact is a **frozen peer** — a
  microcontroller advertising a fixed vocabulary (naming pending).
- **Freeze is a node** on the worker region; peers without toolchains use a
  peer that advertises it (capability placement fallthrough); the artifact
  returns by hash through the plugin gate (msh.graphs_vs_plugins) with recipe provenance —
  the canonical trusted plugin. Unfreezing = reading provenance. Frozen
  programs bypass the bootloader; stage 0 is ultimately the freezer applied
  to the boot graph (sz.frozen_with_provenance's far end).

**frz.round_trip** freeze(graph, target) to artifact with recipe
provenance; unfreeze recovers the editable source graph.
- frz.round_trip.ab_spectrogram: A/B chime interpreted-vs-frozen: spectrogram diff roughly equals 0; block-time
  stat shows the speedup; hot-reload swaps it live.
- frz.round_trip.unfreeze_roundtrip: unfreeze(artifact) yields the source graph hash; re-freezing is a
  memo hit.

**frz.tier_computation** Tier is derived per subgraph from native
closure and shown in the editor.
- frz.tier_computation.tier_derived: hello-cosine's block region reports tier 1; add a tmux node and
  the enclosing graph reports tier 3 with the culprit named.

**frz.freestanding_proof** Tier-1 artifacts compile
`-ffreestanding` with no OS symbols.
- frz.freestanding_proof.freestanding_links: arm-none-eabi build of frozen chime links clean (long before real
  hardware).

**frz.service** Freezing places onto a toolchain-advertising peer and
returns by hash under the plugin gate.
- frz.service.remote_freeze_loads: Quest wires a graph to the remote freeze node; the signed .so
  arrives and hot-loads (msh.graphs_vs_plugins.plugin_gate's flow, provenance-chained to the graph).

## Worked example (test seed)

The projection-editing loop as one scripted test: compile hello-cosine  to 
replace latch with smoother in the realized view to assert app-graph gained
smoother0 (cmp.projection_editing.writeback_smoother) to edit app freq to re-compile to assert smoother survived,
map stable (cmp.determinism_and_map.map_covers_instances), phase continuous (cmp.identity_across_recompilation.phase_continuous) to fork to edit upstream  to 
assert fork untouched (cmp.fork.fork_untouched).
