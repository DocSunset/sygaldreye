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

**CMP-8 (migration path fidelity).** The strangler sequence holds: generated
per-target registration TU → wrap build_plan as one `compile` node
(receive → compile → realize) → retrofit AudioRegion as the first package →
stage-0 extraction → declare extension ports → XR package → web → factor the
`compile` monolith into engine vocabulary.
- CMP-8.1: after step 2, behavior is bit-identical to the pre-tower system on
  the full existing test suite (semantically the current system, new shape).

## Worked example (test seed)

The projection-editing loop as one scripted test: compile hello-cosine →
replace latch with smoother in the realized view → assert app-graph gained
smoother0 (CMP-4.1) → edit app freq → re-compile → assert smoother survived,
map stable (CMP-2.2), phase continuous (CMP-7.1) → fork → edit upstream →
assert fork untouched (CMP-5.1).
