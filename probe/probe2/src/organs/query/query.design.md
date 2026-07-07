# query — the query four (traverse · filter · join · fixpoint)

Owning package: `organs`. Clause: **machinery** (core-named, ADR-024 — the
query four pass the no-composition test as a set). Allowed dependencies:
`parser`, `store`.
A query IS a graph wired from the four primitives; running one is a
committed derivation (memoized index dataset, keyed by query cid + store
version); the standing form maintains its result incrementally — a new
commit seeds a DELTA evaluation through the same pipeline (the monotone
fragment's IVM), so only the dirty cone is recomputed (LNG-10.2).
Fixpoint carries visited-set semantics: cyclic link structures (possible
through refs) terminate (LNG-10.3). Palette and back-link lookups are
expressed as queries through this one evaluator — no bespoke search path
(LNG-10.4).
