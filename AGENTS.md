Be brief. Less is more. Say less.

Write less code. The best code is no code. Every line has a cost.

Make graphs, not C++. If it cannot be a graph yet, the missing vocabulary
is the bug: build the nodes that make it possible, then build it as a
graph.

# Goal

Sygaldreye: a protocol for encoding recursive metadata and decoding data to
derive other data — grounded in stage 0, paced by executors, shared by a
mesh, terminating in someone's senses. Near-term: a live-patchable
audio/visual/XR environment across Linux host, Quest 3, and browser peers.
Far-term: the hypermedium (see `vision.md`).

Run `python3 conformance/run.py` to see the machine gates. But they TRAIL
now — read "How we build now" before treating them as the to-do list.

# How we build now — the third draft (2026-07-07)

Two prior implementations are deprecated probes. **probe1** worked — real
VR, audio, a Quest — but was a coarse C++ monolith built before the dataset
ontology existed. **probe2** took the book + suite to green in a day,
unwatched — and no one, Travis least of all, can say in their heart whether
it is the thing we described. It is still 10k+ lines of C++, some of it
natives that had no right being C++.

So the third draft is paced by one thing: **Travis reads and signs off on
every line.** This is not overhead on the build; it IS the build. The real
project is changing Travis's brain — growing the intuition to hold this
system — and that takes the time it takes. Speed is wanted but is NOT an
acceptance criterion; excitement that outruns understanding buys nothing.

- **50 lines of C++, hard cap.** Never present more than 50 lines of C++ at
  a time. Cross that line and you STOP writing and start explaining yourself.
  Build system, Nix, CMake, shell tooling, Android packaging, **and tests** —
  exempt (Travis doesn't read those); C++ never comes in bigger bites. This is
  the concrete form of "small enough to read and sign in one sitting." Tests
  live in-tree beside their source (`crown.hpp` ↔ `crown.test.cpp`), build and
  run under CMake/CTest, and are held to a looser bar than the rest — Travis
  doesn't care about them and they don't count against the 50-line limit.
- **Travis's comprehension is the clock.** Order slices so each builds the
  next in his head. Bring code to him to READ, not to rubber-stamp.
- **Gate on his taste, not the suite.** He is a C++ veteran — a PhD spent
  building a baby version of this. His judgement covers the volume, not just
  the leaf boundary. Human blessing — "this sounds right / this edit lands /
  this leaf is irreducible" — LEADS as the definition of done; the suite
  trails as regression evidence. Don't follow it to the letter.
- **Explaining is the nativeness gate.** Every C++ native earns its
  nativeness out loud: a plain no-composition argument. If it can't be
  justified cleanly in the telling, it should have been a graph — it dies
  there. (This is "make graphs, not C++" enforced by a human, not a grep.)
- **Sense-first build order.** Escapement → crown → the smallest playable
  graph that drives ONE sense (a tone, a moving dot) with the whole loop
  alive: edit the graph, the world changes, no restart. Get Travis into that
  sandbox as early as possible — that is when building stops being work. VR
  is the destination, not first light.
- **The danger migrates: watch for hollow EXPLANATIONS.** probe2's failure
  was a gameable test suite; this draft's would be prose that makes Travis
  feel he understands without being able to reconstruct it. The test of real
  understanding: he predicts the next line before reading it, and catches the
  code when it's wrong. When his intuition front-runs the code, the brain is
  changing — that is the fresh-eyes audit, relocated into him.
- **If probe2 nailed it, this goes fast.** Much of the third draft may be
  probe2 ported forward with explanation. If it truly is the minimal C++,
  there is little to read. If we keep reaching for C++ nodes that should be
  graphs, that is the alarm — stop and ask what vocabulary is missing.

# Architecture — where truth lives

- **`vision.md`** - What we're really building.
- **`architecture/` — the book.** The sole design document: glossary,
  overview, part chapters with enumerated requirements and acceptance
  criteria, the laws, the greenfield build order. Read its README first.
- **`adr.md`** — ratified decisions (ADR-001…005 are probe-era platform
  choices; ADR-006…028 are the current design). Architectural evolution is
  recorded there and reflected in the book in the same commit.
- **`probe/` is the DEPRECATED DESIGN PROBES** (the entire pre-greenfield
  implementation: components, apps, scripts, assets, its flake; the entire
  fable 5 greenfield implementation: the same sort of stuff again): reference
  and salvage (see the book's appendix), never migrated. Build/run them
  from inside `probe/probeN`.

# Continuity (context is a cache)

The repo is the memory; your context window is a cache of it and must
round-trip. After any compaction, and whenever in doubt, the resume protocol
is: re-read `status.md` in full, then run `python3 conformance/run.py`. Keep
status.md's TOP section a resume block — current rung, in-flight criterion,
next action, active disciplines — updated at every stopping point. Anything
load-bearing that exists only in conversation is already lost; write it down
before it matters.

# Remember

Be brief. Less is more. Say less.

Write less code. The best code is no code. Every line has a cost.

Make graphs, not C++. If it cannot be a graph yet, build the nodes that
make it possible — then build it as a graph.
