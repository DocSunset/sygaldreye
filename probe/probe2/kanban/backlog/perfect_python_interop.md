# Perfect Python interoperability

**Filed:** Travis, 2026-06-30. Bare idea — flesh out when picked up.

The goal: Python should be a first-class citizen of the engine, not a bolted-on
companion. Whatever the C++ node graph can do, Python should be able to do too —
author nodes, drive the graph, read/write endpoints — with no impedance mismatch.

Why: the companion tooling already leans on Python (`companion.py`,
`compile_node.py`, the old voice path). Today that's an outsider talking to the
engine over HTTP/param pokes. "Perfect interop" means erasing that seam.

Open questions to resolve when this comes off the backlog:
- What does "perfect" mean concretely — write nodes in Python? embed a Python
  interpreter in a region? bindings over the C++ ABI? all three?
- Relationship to the plugin ABI (native `.so` nodes) — does a Python node ride
  the same descriptor/endpoint contract?
- Perf boundary: Python out of the audio/worker hot path, or GIL-safe somehow.
