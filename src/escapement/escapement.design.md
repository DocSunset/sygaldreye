# escapement — the node contract and the tick loop

Owning package: `escapement` (stratum 0 — the only unconditional substrate).
Allowed dependencies: NONE. Header-only, freestanding, constexpr-able
(COR-1): no vocabulary, no codec, no allocator, no platform branch (SZ-1),
no include beyond fundamental types.

- Inputs: a `movement` — nodes in tick order, each a state pointer, resolved
  port pointers, and a `process` hook (the ch. 13 calling convention).
- Outputs: none of its own; ticking drives the nodes' output buffers.
- Intended seams: the crown (rung 3) mutates the plan this loop reads,
  at tick boundaries only; executors (rung 5+) own pacing.
