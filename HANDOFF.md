# HANDOFF — to the builder session

You are a fresh agent at the capability level of the one who designed this.
The design phase is CLOSED (2026-07-02 through 07-05, ADR-006…033). Your
mission is to BUILD THE WHOLE THING, greenfield, start to finish: rungs 1
through 12 of `architecture/18-appendix-greenfield.md`, in this repo, until
`python3 conformance/run.py` prints "All gates green. The suite is the
system; the system exists."

This file is peer-to-peer context transfer. `BUILDER.md` is the floor (the
loop, the three rules, the trap list — they bind you too); this file is
what I'd tell myself.

## Authority stack

1. `architecture/10-laws.md` — the laws. 2. `adr.md` — the decisions and
their reasons. 3. The book chapters — requirements and acceptance criteria.
4. `planning/lexicon.md` — the words (retired prose is binding).
`conformance/manifest.json` is DERIVED from the book (extract_manifest.py);
if they disagree, regenerate. If a requirement is genuinely wrong, you do
not route around it: draft an ADR, put it to Travis, and build something
else meanwhile. He ratifies fast and enjoys being disagreed with well —
propose with a recommendation, one concern per message.

## The state of the repo

- `architecture/` — the book, 20 files. Read README → ch. 1 → ch. 16 →
  appendix A before writing anything. Full read before rung 4.
- `conformance/` — 107 requirements / 150+ criteria; zero-dep runner
  (`run.py` — its output is the to-do list); law gates (`gates.sh`, run
  them before every commit); reference ORACLES in `reference/` (executable
  spec for the encoder, address grammar, tape — never port, never import;
  match them byte-for-byte through the `./syg` contract in `HARNESS.md`);
  fixtures (hello-cosine in JSON and tape form; dag-cbor vectors;
  golden-audio properties).
- `src/` — scaffolded, empty: escapement/ crown/ organs/ nodes/, each
  README stating what belongs. Every native's line 1: `// clause:
  machinery | floor | maturity | scaffolding` (ADR-033; gate-checked).
- `probe/` — the sealed previous implementation. Salvage list is in
  appendix A. Genuinely reusable: `probe/components/synth_core` kernels
  (verbatim — they already honor AUT-1), the test suites as golden-corpus
  seeds, and `signal_graph_plan.cpp`/`signal_graph_tick.cpp` as REFERENCE
  READING for plan/tick shape (read, don't copy — the greenfield semantics
  differ: ADR-013 islands, ADR-015 rate-keying).
- `flake.nix` — `nix develop` is your shell. CI runs gates + suite
  (pending fine; failing or uncovered not). Commit per criterion with the
  id in the message. Do NOT `git push` — Travis pushes.
- `kanban/backlog/{transport,companion}_package.md` — post-rung-12 package
  sketches; ignore until then.

## How to actually work (you are capable; the loop still binds)

Work rung by rung; never open rung N+1 with rung N unmet. Within a rung
you may parallelize freely. The tests for rungs 1 and 3 are already
written; from rung 2 on, WRITE THE TEST FIRST from the criterion text,
extend `HARNESS.md` with any new `syg` subcommand in the same commit
(FMT-5 discipline), and replace the stub in `conformance/tests/`. Keep
`manifest.json` fresh (gate enforces). When something resists a full
session: `STUCK.md`, commit, next criterion in the SAME rung.

Session hygiene: update `planning/status.md` at every stopping point
(current rung, criteria green, surprises) — sessions must be resumable by
a stranger. Keep `documentary/log.md` when something story-worthy happens
(decisions, surprises, the first sound — that one especially). Be brief
with Travis; one concern per message; breadcrumb the next step.

## Tacit knowledge the book compresses

- **The generator is the keystone and rung 1's real deliverable.** One
  declaration → in-motion struct + canonical codec + descriptor + JSON
  projection + (later) Python/TS bindings + shells (ADR-017/019, AUT-3,
  ABI-1). Build it as a standalone tool (constexpr/codegen at build time);
  every later rung leans on it. If the generator is right, half the gates
  can never fail. Budget real care here.
- **The plan is segments.** Under ADR-013/015 the executor's inner loop is
  not "for node in topo order": it's a sequence of SEGMENTS — fused
  per-node block loops for feedforward stretches, sample-interleaved SCC
  loops for islands, dirty-gated recompute cones for the value region,
  event applier passes before process. Get the segment representation
  right at rung 5 and EXE-10/11 fall out; bolt it on later and they won't.
- **The crown is smaller than you think.** Mutable table + five op
  appliers + tape reader. Resist giving it policy, transactions, or
  cleverness — those are the full slot's business (rung 4), and the
  tick-boundary application already buys atomicity at control rate.
- **Escapement stays freestanding from day one** (COR-1). It is far easier
  to keep `-ffreestanding` compiling than to regain it. CI it early.
- **Store before compile is deliberate** (rungs 6 → 7): compilation is a
  committed derivation and wants memoization and provenance to exist.
  Don't invert the order to chase the compiler because it's interesting.
- **Determinism classes bite at rung 7**: compile must be exact-class —
  same inputs, byte-identical execution graph AND map. No pointers in
  emitted structures, no iteration over unordered containers, no
  wall-clock anywhere. Cheap to honor from the start, brutal to retrofit.
- **The arbiter is just the edit queue, formalized** (ADR-023). One queue
  per live instance from rung 3 onward; you get multi-writer for free at
  rung 9 if you never let a second mutation path exist.
- **Blessing is a human loop-point**: at rung 2, render the take, send it
  to Travis, he listens and blesses (fixtures/golden-audio.md). Do not
  self-bless. Same at any "does this sound/look right" moment — he is the
  testimony.
- **Rung 7 ends when the engine ticks** (ADR-034, added after the first
  rung-7 pass went hollow). Widen the contract's payload lane FIRST
  (LNG-11 — generated, zero-copy, floats untouched), then factor the
  compile walk into realized pass instances (CMP-9). Do not interpret your
  way past it: a bespoke walk over graph data is scaffolding and must name
  its dissolution criterion. CMP-9.1 is the hollow-engine regression.
- **One peer, many sessions.** From rung 7 on, a `syg` subcommand is a
  session against ONE booted peer (tape → organs → engine slot): every
  mutation is an op into an arbiter, every read is a resolution or query.
  That is CNF-2's candidate-as-peer shape practiced five rungs early — and
  it is what makes nodes-working-with-nodes the path of least resistance.
  If a subcommand can't be expressed that way, an organ is missing; build
  the organ, not a side-door.

## Milestones that matter (report these; skip noise)

R2: hello-cosine SOUNDS (send the take). R3: a tape builds it at runtime.
R5: live edits under golden audio — the guiding star's first real test.
R7: projection editing round-trip (CMP-4.1's smoother survives
re-compilation). R9: two peers, one patch. R10: `import sygaldreye` in a
notebook. R12: sygaldreye-N derives N+1 and the suite runs inside it.

## What you must not do

Reopen the ontology. Reorganize the book. "Improve" ratified vocabulary.
Migrate probe code wholesale. Add a core name without a no-composition
proof. Weaken a test, ever. Push. Pretend a blessing.

The design survived its designers arguing with it for four days; it will
hold your weight. Build it, make it sing at rung 2, and keep the log —
someone will want to tell this story.
