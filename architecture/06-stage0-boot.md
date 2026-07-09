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

**sz.platform_invariance** No platform conditionals in escapement,
crown, or organ sources; per-target variance only in trampoline, TU, tape.
- sz.platform_invariance.ci_grep_gate: CI grep gate. sz.platform_invariance.same_object_all_targets: same object code (per-arch) links into
  host, Quest, and web targets.

**sz.generated_registration** The TU is generated; omission is a loud
link error; the palette equals the manifest.
- sz.generated_registration.missing_native_link_error: deleting a native's object breaks the link naming the symbol.

**sz.naive_resolver** hash to bytes with verification;
no dependency on any package; independently invokable when stage 1 is
wedged (the debugger of last resort).
- sz.naive_resolver.resolver_independent: with the store package broken, the resolver still loads a graph
  by hash from the object directory.

**sz.frozen_with_provenance** The stage-0 artifact links its tape +
native manifest by hash; regeneration via the Nix derivation reproduces it
(or provenance-equal where the toolchain is honest about nondeterminism —
ADR-021 platform-exact).
- sz.frozen_with_provenance.unfreeze_recovers: `unfreeze(stage0)` recovers tape + manifest; rebuild compares.

**sz.spawn_and_park** The parked fallback restarts stage 1 per its
wired policy; stage 0's own graph rejects runtime edits.
- sz.spawn_and_park.restart_survives: kill stage 1 a hundred times; restart each time; sz.spawn_and_park.stage0_rejects_edits: edits addressed at
  stage 0 refused with a clear error.

**sz.trampolines** at most 10 lines each; CI line-count gate.

**sz.boot_without_store** Empty object directory to live engine graph
from embedded tape alone, all targets.

**sz.the_ladder** Escapement + crown + tape reaches full
liveness with zero code paths outside op application (= cor.crown_sufficiency); a
crownless movement build passes movement-level conformance only (cnf.two_profiles).

## Worked example

Cold boot on host: trampoline to escapement to crown replays tape to organs +
supervisor instated to engine slot spawned to audio package spliced (device
observed) to hello-cosine arrives via cli-args stash to compiled to realized  to 
sound. Probe log asserts phase order; then sz.spawn_and_park.restart_survives's crash-restart loop.
