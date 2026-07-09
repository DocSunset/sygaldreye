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
executors: streaming (live tick, recorder off) and derivation mode (run to completion and commit — the worker region is the build system).

**Rate-keyed semantics (ADR-015).** Event ports push (must-not-drop,
same-tick topological propagation in-region, queues across threads). Value
ports are dirty-push + demand-pull: writes propagate staleness; executors,
probes, and derivations demand values; static subgraphs quiesce — lazy but
never stale. Stream ports run clocked, always-hot, dirty-exempt. Clocks are
inputs: time-dependent nodes wire to their executor's published clock, so
"always dirty" is visible dataflow. Effects live in executors and sinks; a
node with no demanded output and no clock input is provably inert (edit-time
lint). This dissolves hot/cold-inlet conventions and trigger-object
sequencing into the kind-and-discipline system and order-is-wiring.

**exe.quiescence_and_demand**
- exe.quiescence_and_demand.static_quiesces: a static scene (no clock-wired nodes, no incoming events)
  performs zero node recomputations across 1,000 frames while still
  presenting (the plan replays cached draw state).
- exe.quiescence_and_demand.dirty_cone_only: one param write recomputes exactly the dirty downstream cone,
  nothing else (counter per node).
- exe.quiescence_and_demand.inert_lint: the inert-node lint flags a node with no demanded output and no
  clock input at edit time.
- exe.quiescence_and_demand.event_wakes_cone: event delivery is unaffected by quiescence: a bang into a
  quiesced subgraph wakes exactly its cone, same-tick semantics preserved.

**Regions are inferred, never declared** — connected components quotiented by
rate, recomputed on every edit; a node's region is the strictest rate among
its ports; declaring both audio and draw_call ports on one node type is an edit-time error (the promise oracle enforces decomposition).

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

**Per-sample islands (ADR-013).** Within a region, the default z-inverse delays ONE
SAMPLE. The planner detects each cycle (strongly-connected island) and
executes it sample-interleaved: per-sample kernels ticked once around the
loop per sample; feedforward stretches keep fused per-node block loops.
Freezing fuses islands to loop-carried variables — a pure optimizer,
identical semantics. Opting out is wiring (an explicit block-sized delay
breaks the island); a cycle through a block-override node forces an explicit
block delay at that edge, loudly, at edit time. Islands are rendered visibly
in the editor, like regions (cost transparency).

**Mappings are visible.** Compilation inserts canonical mappings at inferred
boundaries — z-inverse (cycle), latch (frame to block), snapshot (block to frame), queue
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

**Lifting** (unrelated to compile): excess-rank edges (span producer  to 
cell-kind consumer, not whole) stamp N clones keyed by lift-key cell value or
index; resource holders refuse to lift with a hard, messaged error.

## Requirements

**exe.plan_cache** Build plans exactly as above; `serialize(plan.graph)`
round-trips to the source graph (defaults, never live values).
- exe.plan_cache.pointers_and_latch: hello-cosine's plan holds raw pointers for edges 0 and 2 (true
  edges) and a latch for edge 1; serialize shows the latch in the derived
  mappings array, not in the persisted topology.
- exe.plan_cache.saves_default: saving mid-modulation persists gain's *default*, not the LFO's
  current sample.

**exe.region_inference** Recomputed on every edit; dac upstream closure
through audio edges to block; else frame; queue-implied worker.
- exe.region_inference.infers_membership: hello-cosine infers {osc0, vca0, dac0} block, {lfo0} frame.
- exe.region_inference.edit_moves_region: deleting edge 2 (vca0 to dac0) moves osc0/vca0 out of the block
  region on the very next edit application.

**exe.realtime_safety** The block region allocates nothing and takes no
locks after first-tick init.
- exe.realtime_safety.zero_alloc_lock: instrumented allocator + lock detector report zero events across
  10,000 callbacks of hello-cosine under live param edits.

**exe.canonical_mappings** Latch applies at block start; snapshot
exposes the last completed block; queue never drops events; ring carries
streams SPSC; z-inverse delays exactly one tick with empty-until-first-capture.
- exe.canonical_mappings.latch_at_boundary: step lfo0 to a constant 0.7 to vca0 sees exactly 0.7 from the next
  block boundary, never mid-block.
- exe.canonical_mappings.queue_no_loss: N palette-press events across threads arrive N times, in order.
- exe.canonical_mappings.cycle_delay: a feedback cycle certifies tick-1 delay semantics (existing
  DelayMapping tests inherit here).

**exe.migration** State survives swap keyed by route; clones keyed by
lift key survive reorder and resize.
- exe.migration.phase_continuous: swap hello-cosine for a version with an added noise0: osc0's phase
  is continuous across the swap (record output; no discontinuity beyond one
  block boundary).
- exe.migration.reorder_preserves_state: reordering editor cards (lift_key = id) preserves each card's drag
  state.

**exe.executor_contract** Executors own device + pacing; inner node
families bind through the package's contract; no node reaches for a singleton.
- exe.executor_contract.no_singleton_reach: grep-level check: no `::instance()` reach from node code into
  engine machinery (retires AudioEngine::instance pattern).
- exe.executor_contract.headless_computes: with no audio device, pump_offline paces the block region and
  hello-cosine still computes (headless peers keep computing).

**exe.derivation_mode** A worker-region execution runs a pipeline to
completion and records provenance.
- exe.derivation_mode.offline_render_memoized: "render 2 s of hello-cosine offline" produces a wav dataset with
  recipe provenance; re-running is a memo hit.

**exe.visible_boundaries** Every auto-inserted mapping is present in the
editor's view and replaceable; replacement writes back per cmp.projection_editing.
- exe.visible_boundaries.latch_is_card: the latch in hello-cosine appears as a node card; replacing it
  with `smoother` persists as an app-graph edit and survives re-compilation.

**exe.existence_is_reference** Instances are created by edits that link
them and collected when unlinked; no other lifecycle API.
- exe.existence_is_reference.unlink_collects: removing lfo0 and edge 1 in one edit op collects lfo0's state;
  undo restores it (from the structural snapshot, not the live cell).

**exe.per_sample_islands** Cycle detection per edit;
sample-interleaved execution of islands; explicit block delay opts out;
block-override membership rejected at edit time with a forced explicit delay.
- exe.per_sample_islands.karplus_one_sample: a one-node Karplus-Strong loop (delay-line kernel + damping,
  fed back) sounds correct interpreted: pitch matches 1-sample feedback
  math, not block-delayed math.
- exe.per_sample_islands.block_delay_opts_out: inserting an explicit block-sized delay into the loop reverts
  the island to block execution (observable: per-callback kernel-call count
  drops to per-node-per-block).
- exe.per_sample_islands.override_in_cycle_errors: wiring a spectrogram (block-override) into a cycle yields an
  edit-time error naming the edge that needs an explicit delay.
- exe.per_sample_islands.frozen_equals_interpreted: frozen versus interpreted render of the same feedback patch:
  spectrogram diff roughly equals 0 (the freezer changed cost, not sound).

## Worked example (test seed)

Golden-audio test: realize hello-cosine headless, pump 1 s, assert (a) 220 Hz
peak in FFT, (b) amplitude envelope at 0.5 Hz, (c) exe.realtime_safety counters zero, (d)
swap to freq=330 mid-run and assert phase continuity and new peak — the whole
chapter in one run.
