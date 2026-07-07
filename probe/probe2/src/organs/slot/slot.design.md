# slot — the crown grown up

Owning package: `organs`. Clause: **machinery** (core-named: restructures
the runtime cache transactionally with state migration — ch. 16 stratum 3).
Allowed dependencies: `crown`.
Spec: EXE-5 / ch. 4 migration — replace a plan's graph with a new build-op
sequence; state migrates keyed by ROUTE (same id + same type keeps its
state object; a lifted clone's id carries its lift key, so keyed clones
survive reorder/resize by the same rule); defaults re-apply AFTER
migration; the swap happens at tick boundaries only (atomic from the
ticking thread's view); orphaned state is destroyed, never leaked.
