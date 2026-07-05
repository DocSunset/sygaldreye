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
