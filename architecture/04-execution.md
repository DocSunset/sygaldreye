# Chapter 4 — Execution: plans, regions, mappings, executors (EXE)

*A builder implementing this chapter delivers: the plan compiler-cache, the
tick core, region inference, the canonical mappings, migration, and the
executor contract. Much of this exists in the codebase (signal_graph_plan,
audio_region, event_queue, worker); this chapter states what it must remain
true to as it is reshaped.*

## Design

**The plan is a cache of the graph.** Realization resolves each instance's
`type` link against the registry, calls create() (allocating state), applies
defaults to unconnected inputs, and resolves edges into raw pointers in
topological order. True edges within a region are references — resolved once,
zero per-tick lookups, re-resolved at every swap (the same moment migration
runs). The bag of links is the format; the plan is the cache; serialize must
round-trip.

**Ticking is uncommitted derivation.** Each region ticks its subset at its
cadence: block (hard real-time: no allocation, locks, or syscalls), frame,
worker (may block), net (transport pace). Two pipeline modes on the same
executors: streaming (live tick) and derivation (run-to-completion with
provenance recording — the worker region is the build system).

**Rate-keyed semantics (ADR-015).** Event ports push (must-not-drop,
same-tick topological propagation in-region, queues across threads). Value
ports are dirty-push + demand-pull: writes propagate staleness; executors,
probes, and derivations demand values; static subgraphs quiesce — lazy but
never stale. Stream ports run clocked, always-hot, dirty-exempt. Clocks are
inputs: time-dependent nodes wire to their executor's published clock, so
"always dirty" is visible dataflow. Effects live in executors and sinks; a
node with no demanded output and no clock input is provably inert (edit-time
lint). This dissolves hot/cold-inlet conventions and trigger-object
sequencing into the type system and order-is-wiring.

**EXE-11 (quiescence & demand, ADR-015).**
- EXE-11.1: a static scene (no clock-wired nodes, no incoming events)
  performs zero node recomputations across 10³ frames while still
  presenting (the plan replays cached draw state).
- EXE-11.2: one param write recomputes exactly the dirty downstream cone,
  nothing else (counter per node).
- EXE-11.3: the inert-node lint flags a node with no demanded output and no
  clock input at edit time.
- EXE-11.4: event delivery is unaffected by quiescence: a bang into a
  quiesced subgraph wakes exactly its cone, same-tick semantics preserved.

**Regions are inferred, never declared** — connected components quotiented by
rate, recomputed on every edit; a node's region is the strictest rate among
its ports; declaring both audio and draw_call ports on one node type is a
type error (the type system enforces decomposition).

**Region machinery.** Block membership = dacs plus their upstream closure
through audio edges (a node with audio in but no audio out — a spectrogram —
stays frame-side by design); block instances are excluded from the frame
plan. With no device, `pump_offline` drives blocks from the frame region —
headless peers keep computing. Some kinds pin regions as well as rates
(texture/draw_call pin render — the GL context; audio pins the audio thread).
Cross-region state is carried only by mappings; within a region, appliers run
before process() in topological order, so an event reaches a downstream node
in the same tick — which is why a linear event chain is a legal ordering
declaration (order is wiring).

**Per-sample islands (ADR-013).** Within a region, the default z⁻¹ delays ONE
SAMPLE. The planner detects each cycle (strongly-connected island) and
executes it sample-interleaved: per-sample kernels ticked once around the
loop per sample; feedforward stretches keep fused per-node block loops.
Freezing fuses islands to loop-carried variables — a pure optimizer,
identical semantics. Opting out is wiring (an explicit block-sized delay
breaks the island); a cycle through a block-override node forces an explicit
block delay at that edge, loudly, at edit time. Islands are rendered visibly
in the editor, like regions (cost transparency).

**Mappings are visible.** Compilation inserts canonical mappings at inferred
boundaries — z⁻¹ (cycle), latch (frame→block), snapshot (block→frame), queue
(event cross-thread, never drops), ring (stream cross-thread), net (cross-
peer), probe (observer) — as patchable nodes, replaceable by the user, never
persisted (derived, shown as derived).

**Executors own pacing and the platform resource.** An executor is a node
holding an inner graph plus machinery (thread, device callback, frame
envelope). Inside, the story recurses. Allocation discipline: no resource
acquisition in create(); first-tick lazy init; fail loud when ticked without
context.

**Migration.** The slot primitive — replace the subgraph of slot X, migrating
state — keys state by route (instances) and lift key (clones); defaults
re-apply; the swap is atomic from the ticking thread's view. Exec and spawn
are its two call sites; spawn parks a fallback with restart policy
(Erlang-shaped supervision).

**Lifting** (unrelated to compile): excess-rank edges (span producer →
cell-kind consumer, not whole) stamp N clones keyed by lift-key cell value or
index; resource holders refuse to lift with a hard, messaged error.

## Requirements

**EXE-1 (plan cache).** Build plans exactly as above; `serialize(plan.graph)`
round-trips to the source graph (defaults, never live values).
- EXE-1.1: hello-cosine's plan holds raw pointers for edges 0 and 2 (true
  edges) and a latch for edge 1; serialize shows the latch in the derived
  mappings array, not in the persisted topology.
- EXE-1.2: saving mid-modulation persists gain's *default*, not the LFO's
  current sample.

**EXE-2 (region inference).** Recomputed on every edit; dac upstream closure
through audio edges → block; else frame; queue-implied worker.
- EXE-2.1: hello-cosine infers {osc0, vca0, dac0} block, {lfo0} frame.
- EXE-2.2: deleting edge 2 (vca0→dac0) moves osc0/vca0 out of the block
  region on the very next edit application.

**EXE-3 (real-time safety).** The block region allocates nothing and takes no
locks after first-tick init.
- EXE-3.1: instrumented allocator + lock detector report zero events across
  10⁴ callbacks of hello-cosine under live param edits.

**EXE-4 (canonical mappings).** Latch applies at block start; snapshot
exposes the last completed block; queue never drops events; ring carries
streams SPSC; z⁻¹ delays exactly one tick with empty-until-first-capture.
- EXE-4.1: step lfo0 to a constant 0.7 → vca0 sees exactly 0.7 from the next
  block boundary, never mid-block.
- EXE-4.2: N palette-press events across threads arrive N times, in order.
- EXE-4.3: a feedback cycle certifies tick-1 delay semantics (existing
  DelayMapping tests inherit here).

**EXE-5 (migration).** State survives swap keyed by route; clones keyed by
lift key survive reorder and resize.
- EXE-5.1: swap hello-cosine for a version with an added noise0: osc0's phase
  is continuous across the swap (record output; no discontinuity beyond one
  block boundary).
- EXE-5.2: reordering editor cards (lift_key = id) preserves each card's drag
  state.

**EXE-6 (executor contract).** Executors own device + pacing; inner node
families bind through the package's contract; no node reaches for a singleton.
- EXE-6.1: grep-level check: no `::instance()` reach from node code into
  engine machinery (retires AudioEngine::instance pattern).
- EXE-6.2: with no audio device, pump_offline paces the block region and
  hello-cosine still computes (headless peers keep computing).

**EXE-7 (derivation mode).** A worker-region execution runs a pipeline to
completion and records provenance.
- EXE-7.1: "render 2 s of hello-cosine offline" produces a wav dataset with
  recipe provenance; re-running is a memo hit.

**EXE-8 (visible boundaries).** Every auto-inserted mapping is present in the
editor's view and replaceable; replacement writes back per CMP-4.
- EXE-8.1: the latch in hello-cosine appears as a node card; replacing it
  with `smoother` persists as an app-graph edit and survives re-compilation.

**EXE-9 (existence = reference).** Instances are created by edits that link
them and collected when unlinked; no other lifecycle API.
- EXE-9.1: removing lfo0 and edge 1 in one edit op collects lfo0's state;
  undo restores it (from the structural snapshot, not the live cell).

**EXE-10 (per-sample islands, ADR-013).** Cycle detection per edit;
sample-interleaved execution of islands; explicit block delay opts out;
block-override membership rejected at edit time with a forced explicit delay.
- EXE-10.1: a one-node Karplus-Strong loop (delay-line kernel + damping,
  fed back) sounds correct interpreted: pitch matches 1-sample feedback
  math, not block-delayed math.
- EXE-10.2: inserting an explicit block-sized delay into the loop reverts
  the island to block execution (observable: per-callback kernel-call count
  drops to per-node-per-block).
- EXE-10.3: wiring a spectrogram (block-override) into a cycle yields an
  edit-time error naming the edge that needs an explicit delay.
- EXE-10.4: frozen vs interpreted render of the same feedback patch:
  spectrogram diff ≈ 0 (the freezer changed cost, not sound).

## Worked example (test seed)

Golden-audio test: realize hello-cosine headless, pump 1 s, assert (a) 220 Hz
peak in FFT, (b) amplitude envelope at 0.5 Hz, (c) EXE-3 counters zero, (d)
swap to freq=330 mid-run and assert phase continuity and new peak — the whole
chapter in one run.
