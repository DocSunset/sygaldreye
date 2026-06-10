# Concurrency Design Notes

> **Status: incomplete.** This records an exploratory discussion. Key questions are unresolved and the design is not ready to implement.

## Motivation

The signal graph currently runs single-threaded: one `tick_graph` call, topologically sorted, sequential. The app needs at minimum:

- An audio thread running at ~48 kHz (block-rate OS callback, ~5 ms blocks)
- A VR render thread at ~72 Hz (~14 ms, driven by `xrWaitFrame`)
- Control data arriving aperiodically, best-effort, possibly timestamped

These three execution contexts run concurrently and exchange data. The current value-copy approach through `Graph::values` is not thread-safe and is not designed for this.

## Terminology

**Executor**: a thread plus its tick trigger. An executor owns a graph and decides when to call `tick_graph`. The three executors above are peers — none is "outer" relative to the others.

**Graph**: the static topology (nodes and edges). A graph belongs to exactly one executor. This is distinct from the existing subgraph/nesting concept.

**Subgraph node**: a node whose `process` calls `tick_graph` on an inner graph synchronously on the calling thread. A composition mechanism only — no threading involved.

## Edge taxonomy

The zero-copy backlog design (`zero_copy_graph_edges.md`) introduces same-thread pointer wiring. Cross-executor data transfer requires additional edge kinds. Ownership of the synchronization mechanism lives in the edge.

| Kind | Threading | Semantics | Mechanism |
|---|---|---|---|
| sync | same executor | zero-copy pointer | direct struct field wiring (see backlog) |
| signal | cross-executor | latest value wins; consumer never blocks | seqlock or atomic triple-buffer |
| event | cross-executor | all values matter; FIFO | lock-free SPSC queue |

The `signal` edge is *not* a size-1 queue. A size-1 queue still has ownership/failure semantics on enqueue. A signal edge requires the producer to always succeed and the consumer to always read the latest complete value — a seqlock or triple-buffer, not a queue.

MoodyCamera's `concurrentqueue` (`$CONCURRENTQUEUE_INCLUDE_DIR`) is available in the dev shell as a reference for the event edge implementation. Exact overflow/blocking behavior of a size-1 instance has not been confirmed.

## Concrete data flows (partial)

| Producer | Consumer | Edge kind | Notes |
|---|---|---|---|
| render (head pose) | audio (HRTF) | signal | audio interpolates/extrapolates between render ticks |
| control (parameter) | audio | signal | apply at next block boundary |
| control (parameter) | render | signal | |
| audio (level meter) | control/render | signal | render reads latest, no history needed |
| control (note-on, UI event) | audio or render | event | missing an item is wrong |

## Open questions

- **Control executor**: what is its tick trigger? Poll loop, blocking select on incoming socket/MIDI fd, or purely reactive (wakes on data)? This determines whether it even has a `tick_graph` loop or is purely push-driven.
- **Timestamped control data**: if control messages carry sample-accurate timestamps, the audio executor needs scheduled parameter application — a qualitatively different feature. Is "apply at next block boundary" sufficient, or is sample-accurate scheduling required?
- **Rate mismatch on event edges**: if the audio executor produces events faster than the control executor drains them, does the queue bound? Drop? Block? Policy unresolved.
- **Graph declaration**: do executor affinity and edge kind get declared in the JSON graph schema, or inferred from topology? Inferred is elegant but hard to debug.
- **Render → audio head pose**: audio runs faster than render (~183 blocks/s vs 72 frames/s). Audio needs a pose every block. Does it extrapolate, hold last, or interpolate using a ring of recent poses? Unresolved.
- **Zero-copy backlog first**: the pointer-wiring design should probably land before cross-executor edges are built on top of it, since sync edges are the foundation.
