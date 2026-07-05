# Chapter 6 — Stage 0 and boot (SZ)

*Rewritten 2026-07-04 for ADR-028. A builder implementing this chapter
delivers: the escapement, the crown, the per-target trampolines and
generated registration TU, the boot tape, and the climb to a live peer.*

## Design

**Stage 0 is a frozen realization** — ordinary nodes whose instantiation
happened at build time (a Nix derivation; provenance to the flake lock;
fiat retreats to the toolchain/physics boundary). Its one irreducible
property is pre-existence. Composition, by stratum (ch. 16):

- **The escapement**: node contract + tick-in-order. All any target must
  contain. A sealed firmware is escapement + **movement** (frozen graph),
  full stop.
- **The crown**: the plan as mutable data + one applier primitive (ops at
  tick boundaries) + an op inlet. The minimal self-modification; omitted on
  sealed movements.
- **The boot tape**: the boot "graph" as a flat sequence of fixed-format op
  records the crown replays (a graph is equivalent to its building ops, ADR-018) — no
  parser, codec, or resolver required to reach a running, modifiable peer.
- **Linked liveness organs** (per target, by the generated registration
  TU): parser, codec, naive resolver, registry-face, ref, subgraph, slot,
  reflection, query four — nodes like all nodes, instated by the tape when
  the target wants liveness (ch. 16 stratum 3).
- **The platform stash**: opaque; only platform natives read it; entry
  symbols are <10-line trampolines (main / android_main / emscripten /
  `import sygaldreye` — ADR-019 made the module loader a trampoline too).

**Invariant unchanged**: stage-0 logic never branches on platform;
per-target variance lives only in trampoline, registration TU, and tape.

**Boot sequence** (hosted default): trampoline to escapement ticks to crown
replays the tape to tape instates the liveness organs and a supervisor
subgraph to spawns the engine graph (data: `receive to compile to realize`)
into a slot and parks as fallback (the supervision base case, ADR-016)  to 
the engine observes the environment, splices capability packages through
its fan-ins, receives graphs from instruction sources, compiles, realizes.
Hosted default tapes include the testimony buffer + death-watch nodes;
freestanding tapes omit them (ADR-016). Frozen movements bypass all of
this by design; ultimately the codegen backend applied to the boot tape
IS how stage 0 is built (ADR-014 and 027).

## Requirements

**SZ-1 (platform invariance).** No platform conditionals in escapement,
crown, or organ sources; per-target variance only in trampoline, TU, tape.
- SZ-1.1: CI grep gate. SZ-1.2: same object code (per-arch) links into
  host, Quest, and web targets.

**SZ-2 (generated registration).** The TU is generated; omission is a loud
link error; the palette equals the manifest.
- SZ-2.1: deleting a native's object breaks the link naming the symbol.

**SZ-3 (naive resolver, hosted default).** hash to bytes with verification;
no dependency on any package; independently invokable when stage 1 is
wedged (the debugger of last resort).
- SZ-3.1: with the store package broken, the resolver still loads a graph
  by hash from the object directory.

**SZ-4 (frozen-with-provenance).** The stage-0 artifact links its tape +
native manifest by hash; regeneration via the Nix derivation reproduces it
(or provenance-equal where the toolchain is honest about nondeterminism —
ADR-021 platform-exact).
- SZ-4.1: `unfreeze(stage0)` recovers tape + manifest; rebuild compares.

**SZ-5 (spawn-and-park).** The parked fallback restarts stage 1 per its
wired policy; stage 0's own graph rejects runtime edits.
- SZ-5.1: kill stage 1 a hundred times; restart each time; SZ-5.2: edits addressed at
  stage 0 refused with a clear error.

**SZ-6 (trampolines).** at most 10 lines each; CI line-count gate.

**SZ-7 (boot without store).** Empty object directory to live engine graph
from embedded tape alone, all targets.

**SZ-8 (the ladder, ADR-028).** Escapement + crown + tape reaches full
liveness with zero code paths outside op application (= COR-2); a
crownless movement build passes movement-level conformance only (CNF-3).

## Worked example

Cold boot on host: trampoline to escapement to crown replays tape to organs +
supervisor instated to engine slot spawned to audio package spliced (device
observed) to hello-cosine arrives via cli-args stash to compiled to realized  to 
sound. Probe log asserts phase order; then SZ-5.1's crash-restart loop.
