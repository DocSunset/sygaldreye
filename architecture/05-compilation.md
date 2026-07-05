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

**Compilation is a derivation** (inputs: app graph hash + engine graph hash →
execution graph dataset), so it memoizes like any derivation. It must be
**deterministic** and emit the **compilation map** (app route → execution
route) — the enabler of state migration across re-compilation and of
projection editing. Deterministic = compilation assigns stable local names
(the lift machinery's "key by cell value, else index," stated generally).

**Extension points are ports.** The initial engine graph publishes fan-ins —
recognize-region, construct-context, choose-adapters (more may be surfaced as
the `compile` monolith is factored) — and capability packages *wire into
them*. Raw graph→graph surgery remains possible (a splice is an ordinary
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

**CMP-1 (compile as derivation).** compile(app, engine) → execution dataset
with recipe provenance; memoized on input hashes.
- CMP-1.1: compiling hello-cosine twice runs passes once (counter).
- CMP-1.2: editing the *defaults* node only does not re-run structural passes
  (topology hash unchanged → memo hit on the structural stage).

**CMP-2 (determinism + map).** Same inputs → identical execution graph hash
AND identical compilation map.
- CMP-2.1: compile hello-cosine 100×; one distinct output hash.
- CMP-2.2: the map contains `nodes/osc0 → block/osc0` (or equivalent stable
  scheme) and every app instance appears exactly once.

**CMP-3 (extension ports).** Packages integrate exclusively by wiring into
published fan-ins in the well-behaved path; the audio package passes this bar.
- CMP-3.1: with the audio package spliced, the engine graph diff against
  vanilla is additive wiring only (no rewrites of existing engine nodes).
- CMP-3.2: pass execution order equals topological order of the engine patch
  (probe records order; compare to wiring).

**CMP-4 (projection editing).** Edits in realized views write back through
the inverse map; insert-if-absent for default mappings.
- CMP-4.1: replace hello-cosine's latch with `smoother` in the execution
  view → the app graph now contains smoother0 wired at edge 1; re-compiling
  inserts NO latch there.
- CMP-4.2: an edit whose target route vanished upstream surfaces a conflict
  (not silence, not a crash).

**CMP-5 (fork).** Rebinding a ref away from a derivation's output records
detachment; upstream re-compilation no longer applies; the fork is visible in
lineage queries.
- CMP-5.1: fork hello-cosine's execution graph, edit upstream app graph,
  re-compile: the forked ref is untouched and lineage marks the detachment.

**CMP-6 (laziness).** No engine graph above the level being edited is
resident.
- CMP-6.1: steady-state hello-cosine playback instantiates zero engine
  instances (only stage 0's parked loop + the running execution graph);
  opening the engine editor instantiates exactly one level.

**CMP-7 (identity across re-compilation).** State survives app-graph edits
end-to-end (composition of CMP-2's map with EXE-5's migration).
- CMP-7.1: while sounding, add noise0 to hello-cosine, re-compile, swap:
  osc0's phase continuous; latch state preserved.

**CMP-8 (RETIRED 2026-07-04).** The strangler migration path assumed
refactoring the probe; the ratified greenfield build (appendix,
18-appendix-greenfield.md) supersedes it. The probe is reference and
salvage, never migrated. Kept as a numbered tombstone so citations resolve.

## Freezing (FRZ)

Freezing is a derivation whose output kind is C++ — ratified (ADR-014): a
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
  anywhere C++ runs) · tier 2 platform-lib (GLES/audio hosts) · tier 3
  host-bound. A tier-1-plus-net artifact is a **frozen peer** — a
  microcontroller advertising a fixed vocabulary (naming pending).
- **Freeze is a node** on the worker region; peers without toolchains use a
  peer that advertises it (capability placement fallthrough); the artifact
  returns by hash through the plugin gate (MSH-5) with recipe provenance —
  the canonical trusted plugin. Unfreezing = reading provenance. Frozen
  programs bypass the bootloader; stage 0 is ultimately the freezer applied
  to the boot graph (SZ-4's far end).

**FRZ-1 (round-trip).** freeze(graph, target) → artifact with recipe
provenance; unfreeze recovers the editable source graph.
- FRZ-1.1: A/B chime interpreted-vs-frozen: spectrogram diff ≈ 0; block-time
  stat shows the speedup; hot-reload swaps it live.
- FRZ-1.2: unfreeze(artifact) yields the source graph hash; re-freezing is a
  memo hit.

**FRZ-2 (tier computation).** Tier is derived per subgraph from native
closure and shown in the editor.
- FRZ-2.1: hello-cosine's block region reports tier 1; add a tmux node and
  the enclosing graph reports tier 3 with the culprit named.

**FRZ-3 (freestanding proof).** Tier-1 artifacts compile
`-ffreestanding` with no OS symbols.
- FRZ-3.1: arm-none-eabi build of frozen chime links clean (long before real
  hardware).

**FRZ-4 (service).** Freezing places onto a toolchain-advertising peer and
returns by hash under the plugin gate.
- FRZ-4.1: Quest wires a graph to the remote freeze node; the signed .so
  arrives and hot-loads (MSH-5.1's flow, provenance-chained to the graph).

## Worked example (test seed)

The projection-editing loop as one scripted test: compile hello-cosine →
replace latch with smoother in the realized view → assert app-graph gained
smoother0 (CMP-4.1) → edit app freq → re-compile → assert smoother survived,
map stable (CMP-2.2), phase continuous (CMP-7.1) → fork → edit upstream →
assert fork untouched (CMP-5.1).
