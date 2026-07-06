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

## CMP-9.2 + EXE-11.4 — 2026-07-05 (remediation audit)
The remediation audit demonstrated a stale-memo hole: live set_text /
set_param edits never reached the plan's persisted doc, so the compile
recipe under-covered the engine definition — blanking a rule's text
memo-hit the OLD execution. Fixed at the root (L13): param ops now fold
into doc defaults (and die with their node on remove_node); the receive
inlet is stripped from the engine identity (the app rides the recipe as
its own input). cmp92 gained the regression: a param-only engine edit
must miss the memo and undo the placement. Also killed rung05's
`== {} or True` vacuous assert (exe114 now pins the watched VALUES) and
tightened the dissolution gate (verb/case variants, trailing digits,
.c/.cc/.h/.hh/.inl/.py coverage).

## AUT-5.1 — 2026-07-05
Criterion asks "same golden audio within tolerance" across the four
authoring routes; the test asserts BYTE-IDENTITY of all three swapped
renders against the native baseline (subgraph expansion, frozen artifact
via its emitted node-type contract, shipped plugin through the gate),
plus golden properties on the baseline and one-registry promise checks.

## LNG-2.2 fixture amendment — 2026-07-05 (PKG-4 retrofit)
instanced_draw was self-clocked ("presents per frame" via clocked=true) —
exactly the "always dirty as node property" ADR-015 retired. The present
is now driven by render_head's chain (clocks are INPUTS, visible as
wiring). LNG-2.2's fixture gains the head + one edge; every assertion
unchanged. PKG-4.2 pins the consequence: an unchained draw renders zero.

## FRZ-1.1 — 2026-07-05 (rung-8 audit, blocker fixed)
The scc_order extraction was NOT verbatim: cut delay edges lost their
ordering role, so scheduling fell to name order (auditor's probe: renaming
pulse0→apulse0 moved a 200-sample delay's impulse to 328 interpreted /
201 frozen). Fixed: Tarjan runs on the cut set, the condensation honors
ALL edges (cut edges as soft constraints, broken only by real cycles, in
index order — never by name); a cycle-forced cut edge now means ONE BLOCK
of latency in BOTH backends (codegen grew double-buffered block-carry).
frz11 pins three regression shapes: adversarial-name straight-line cut
(impulse at exactly 200), in-cycle cut (echoes at 0/328/656, byte-equal),
and the frozen frame/latch path (hello A/B — previously never rendered
frozen). Codegen also now REFUSES loudly what it would silently drop
(unknown frame emitters, unbakeable defaults); tier>1 reports without
emitting.

## MSH-5.1 — plugin gate: witness the bad-signature branch (2026-07-05, rung-9 audit)

The rung-9 fresh-context audit demonstrated that MSH-5.1's negative cases
(unsigned, untrusted-signer) never exercised the plugin gate's SIGNATURE
check: with `verify()` neutered, MSH-5.1 still passed (the shared primitive
was witnessed only by MSH-6.1). Added a forged-signature case — a TRUSTED
signer's signature corrupted in transit (new test-only `forge_sig` knob on
`ship-plugin`) — asserting the gate rejects at `bad-signature`, not on trust
alone. Now neutering `verify()` turns BOTH MSH-6.1 and MSH-5.1 red.
