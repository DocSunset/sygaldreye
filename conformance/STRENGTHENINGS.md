# Strengthenings ledger (BUILDER.md Judgement §1)

One entry per unilateral tightening of the conformance regime. Weakening
is ADR-gated, always; nothing here may relax an assertion.

- 2026-07-05 · LNG-1.1 — the catalog's expected set grew by `graph`, `ops`, and
  `cidset` (ADR-034's payload kinds; ch. 11's list was the stale side of
  the ADR-wins rule and was fixed in the same commit). The assertion is
  still exact-set equality — a larger set to match exactly is a tightening,
  not a relaxation.
- 2026-07-05 · LNG-11.1 — beyond the criterion text, the test also asserts
  the oracle refuses structured kinds on BOTH directions of a stream
  crossing, and pins the zero-encode witness (encode-counter delta == 0),
  not merely "no serialization observed".
- 2026-07-05 · LNG-10.2 — rewritten for the realized standing query
  (LNG-11.4): instead of a full-vs-delta eval-count proxy, the test now
  proves cone ISOLATION directly — an out-of-cone control chain sharing
  the live plan recomputes zero times while the seeded cone recomputes.
  Strictly stronger: the old proxy could pass with the control chain
  churning.

## LNG-11.1 — 2026-07-05 (rung-7 audit, blocker)
The oracle's structured-kind mirror omitted `cidset`: a cidset legally rode
the block clock (empirically shown, `mapping: snapshot`). Three hand copies
existed (kinds.json, crown.hpp, oracle.cpp); the catalog itself lacked
`text`. Fix: the generator now emits `structured_kinds.hpp` from
vocabulary/kinds.json — the ONE copy, honoring the catalog's own charter
("read as data by the oracle; no hardcoded switch anywhere"); both hand
copies deleted. Test strengthened: probes EVERY structured kind in the
catalog, both directions, instead of graph only.

## CMP-9.2 — 2026-07-05 (rung-7 audit)
The rules lane was inert transport: nothing consumed `__rules`, so the
spliced pass changed only an echoed string (hash change without compile
consequence). Now `block:<id>` rules claim an instance for the block
region in choose-adapters, and the test asserts lfo0's placement actually
moves (frame → block) and moves back on unsplice. realize's `backend`
port is read instead of decorative.

## CMP-9.2 dissolution + CMP-1.2 — 2026-07-05 (rung-7 audit, blocker)
The bespoke walk (compiler.cpp) still backed 11 green criteria under a
false `dissolves: CMP-9.2` marker. Retired for real: `syg compile` now
runs the SAME realized engine plan as `syg peer` (shared core,
realized_compile.cpp); compiler.cpp is deleted. The structural stage is
realized as a committed store derivation inside the choose-adapters hook
(keyed on topology + rules); recognize-region short-circuits to identity
when nothing can expand. CMP-1.2's `structural_memo` is now derived from
an abi counter of actual expansion/inference runs — the test additionally
pins that the FIRST compile does structural work (a dead counter fails).
`passes_run` became the executor's tick trace. New runner gate: a live
`dissolves:` marker naming a GREEN criterion fails the suite.

## CMP-6.1 — 2026-07-05 (rung-7 audit)
`engine_alive` was a hand tally in the session code it audited. Now an
executor-side census: exec_plan counts live plans whose expanded doc
contains a realize instance; sessions just read it.
