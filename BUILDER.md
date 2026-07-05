# To whoever builds this

You may be human or an agent; you may be less experienced than the people
who designed this. That is fine — the design assumed it. Your job never
requires deciding WHAT to build or WHETHER it's right. Both are decided,
recorded, and mechanically checkable. Your job is to make red tests green,
in order.

## The loop (this is the whole method)

1. Run `python3 conformance/run.py`. It tells you which rung you are on and
   names the first pending or failing criterion.
2. Read that criterion's chapter section in `architecture/` (the manifest
   entry names the chapter).
3. Write exactly enough — first the test if it's pending, then the code —
   to turn that one criterion green. No more.
4. Run `conformance/gates.sh` and the runner again. Commit. Repeat.

The build order is `architecture/18-appendix-greenfield.md`. Do not skip
rungs. Do not start rung N+1 while rung N's gate is unmet.

## The three rules

1. **Never edit a law, a requirement, or a test to make something pass.**
   If a requirement seems wrong, STOP and record the problem as a proposed
   ADR (see `adr.md` for the form); the book and manifest change first,
   together, or not at all. A green suite obtained by weakening a test is
   worthless — worse, it is a lie with provenance.
2. **When in doubt, build less.** Every line has a cost. If two designs
   pass the test, ship the smaller one. The core only shrinks (L18); you
   are never authorized to add to `architecture/16-the-core.md`.
3. **Everything you make must round-trip.** If you write bytes by hand, or
   serialize outside the generated codec, you have already failed FMT-1 —
   even if the test hasn't caught you yet.

## The trap list — shortcuts that feel right and violate the law

- A global values map or ambient store singleton (violates L1/store-as-graph;
  killed twice already, stays dead).
- Hand-written try/catch or serializers in node code (ABI-1; the generator
  writes those).
- Snapshot-based undo (ADR-018: history is the op log).
- "Just this once" platform `#ifdef` in core sources (SZ-1).
- Resource acquisition in create() (L9: first-tick lazy, fail loud).
- Making a hash resolve to something mutable (impossible if you use the
  verified fetch path; if you found another path, that's the bug).
- Blocking anywhere except the worker region (ADR-016).
- Skipping the compilation map "for now" (CMP-2; state migration and
  projection editing both die without it).

## Where things are

- `architecture/` — the book. The README says how to read it. Chapter 10 is
  the law; chapter 16 is the core; the appendices are your build order and
  your ancestors.
- `adr.md` — every decision, with reasons. ADR-013 through 031 govern.
- `conformance/` — the manifest (regenerate from the book, never hand-edit),
  the runner, the gates, the fixtures. `fixtures/hello-cosine.*` is the
  canonical example you will make sound at rung 2.
- `probe/` — the sealed previous implementation. Reference and salvage only:
  `probe/components/synth_core` kernels survive verbatim; its tests seed the
  golden corpus. Never migrate probe code wholesale.
- `planning/lexicon.md` — the vocabulary. Use the ratified words; retired
  prose is binding.

## Your environment

`nix develop` at the repo root gives you the toolchain. CI runs
`conformance/gates.sh` and the runner on every push — pending is fine,
failing or uncovered is not. The tests for the early rungs are ALREADY
WRITTEN: `conformance/reference/` holds slow, obviously-correct oracle
implementations (they ARE the spec — never port or import them), and your
implementation talks to the suite through the `./syg` binary contract in
`conformance/HARNESS.md`. You do not need to understand canonical CBOR;
you need to match the oracle, byte for byte, and the test will tell you
the first value you get wrong.

## When you are stuck

If a criterion resists for more than a working session: write `STUCK.md`
at root — the criterion id, what you tried, the exact error — commit it,
and move to the NEXT criterion in the SAME rung. Never to the next rung;
never by weakening the test. A recorded dead-end is recoverable work;
a silent workaround is damage.

## What done means

Rung 12 green: the suite runs as datasets inside the system it verifies, and
sygaldreye-N can derive sygaldreye-N+1. Before that, "done" for any session
is: the runner's YOU ARE HERE moved forward, gates green, work committed
with the criterion id in the message.

If you keep the three rules, the worst you can build is something small,
honest, and unfinished — which is recoverable. The design already survived
its designers arguing with it; it will survive you. Good luck, and make it
sing — literally, at rung 2.
