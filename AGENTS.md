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
Far-term: the hypermedium (see `planning/vision.md`).

Run `python3 conformance/run.py` — its output is the to-do list.**

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
