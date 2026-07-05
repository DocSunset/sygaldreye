# Chapter 12 — Node authoring and conformability (AUT)

*How new vocabulary is made, and how scalar behaviors conform over channels,
frames, and instances. A builder implementing this chapter delivers: the
kernel contract, the shell stamps, the endpoints authoring surface, and the
lifting machinery's guarantees.*

## Design

**The kernel contract (ratified, kernel-extraction arc).** Every audio ugen's
DSP is a per-sample KERNEL: a plain struct, state + `tick()`, no buffers, no
node machinery. Rules:

- Per-sample tick; params are plain members written before the loop (latched
  per block).
- NO absolute-time state (the metro epoch bug class): time arrives as dt or
  per-sample increments; dt clamping lives in the SHELL, never the kernel.
- One kernel instance per channel = the conformability unit.
- POD-friendly: no allocation in tick(); delay lines size buffers in
  prepare(rate, max), never per tick.
- Purity tiers: "freestanding" = cmath only, no OS, no allocation after init
  (required for freeze tier 1; all current kernels qualify).

Kernels live in synth_core; ADR-013 makes them load-bearing for the
*interpreter* (per-sample islands), not just the freezer.

**The shell stamps (both halves have a compile-time stamp).** PROCESSORS:
`Lift<K>` — one kernel instance per channel (map axis) scanning frames (time
axis). GENERATORS: `Gen` — the source authors a per-sample body; the shell
owns dt to frame clamping and the mono buffer. Documented exceptions only:
metro (time/event node — per-sample gate-edge timestamps), spatialize
(axis-consuming whole-block HRTF), dac (resource holder — never lifted). No
hand-written block loops outside these.

**The endpoints authoring surface.** A node type is authored as one C++
endpoints struct (reflection via PFR — real named fields, so endpoint structs
stay authored); the descriptor machinery GENERATES the one-level-up
declarations (ports, types, affordance metadata) from the struct. The strong
compiler verifies inside nodes; the first-order oracle verifies between them;
the generated descriptor is the border crossing, so it cannot drift (L3, L7).
Convention: processor audio in = `audio`, out = `audio_out`.

**Lifting (conformability).** The executor cannot lift what it cannot see
inside opaque descriptors, so lifting lives in the descriptor machinery and
the planner:

- Excess-rank detection at plan time (span producer to cell-kind consumer,
  not whole) creates a LiftGroup; the plan replays it via N desc to create()
  clones — builtins AND subgraphs — outputs gathered into a plan-owned span.
- Clone state survives reorder, resize, and migration, keyed by the lift-key
  cell value when a key source is wired (a card keyed by node id), else by
  index (audio channels).
- Stateless cell-kind maps loop without clones (CellMap).
- Resource holders refuse to lift with a hard, messaged error; subgraphs
  infer resource-holder-ness from their interior.
- GPU instancing is the render boundary consuming a span whole (LNG-2.2) —
  instancing is not a strategy, it falls out of rank.

**Authoring beyond C++.** A subgraph dataset is a node type (LNG-6); a frozen
artifact is a node type (ch. 5); a shipped plugin is a node type under the
trust gate (MSH-5). All four authoring routes — native struct, subgraph,
frozen artifact, plugin — land in the same registry and are indistinguishable
to a patch.

## Requirements

**AUT-1 (kernel contract).** Kernels are per-sample, absolute-time-free,
allocation-free in tick, unit-tested directly.
- AUT-1.1: synth_core kernel suite green; a kernel holding wall-clock state
  fails review by grep (`time(`, epoch fields) and the metro exception is
  documented in-source.
- AUT-1.2: every kernel ticks correctly at N=1 sample granularity (island
  readiness — ADR-013).

**AUT-2 (shell stamps).** No hand-written block loops outside the documented
exceptions; both stamps preserve block semantics exactly.
- AUT-2.1: grep gate for raw frame loops in ugens; exceptions annotated.
- AUT-2.2: ugens test suite unchanged across a stamp refactor (the
  kernel-extraction acceptance, kept green forever).

**AUT-3 (generated descriptors).** Port declarations, types, and affordances
are generated from endpoint structs; no hand-maintained descriptor tables.
- AUT-3.1: adding a field to an endpoints struct surfaces the port, its
  type, and its widget with zero descriptor edits.

**AUT-4 (lift guarantees).** Keyed clone state survives reorder/resize/
migration; index-keyed suffices for channels; resource holders refuse.
- AUT-4.1: EXE-5.2 (cards by id) and LNG-2.1 (channels by index) both pass;
  LNG-6.2's refusal message names the culprit.
- AUT-4.2: a span into a lift-key port does not steal the lift (the
  rung-3 selection bug's regression test).

**AUT-5 (four routes, one registry).** Native, subgraph, frozen, and plugin
node types are palette-identical and patch-interchangeable.
- AUT-5.1: hello-cosine's osc swapped for (a) a subgraph osc, (b) a frozen
  osc artifact, (c) a shipped plugin osc — same patch JSON otherwise, same
  golden audio within tolerance.

## Worked example (test seed)

The forest, both routes: (route 2) one tree mesh + an N-by-3 positions span into
the instanced draw — one draw call (LNG-2.2); (route 1) an N-seed span into a
lifted tree_generator — N distinct meshes, clone state keyed by seed
(AUT-4.1); resize N live and watch identity hold. The same test then swaps
tree_generator for its frozen artifact (AUT-5.1).
