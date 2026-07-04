# Chapter 6 — Stage 0 and boot (SZ)

*A builder implementing this chapter delivers: the frozen kernel, the
per-target trampolines and generated registration TU, the embedded boot graph,
and the boot sequence into stage 1.*

## Design

Stage 0 is **a frozen realization of the boot graph**: ordinary native nodes
whose parsing, planning, and instantiation happened at *build time* (a Nix
derivation — provenance continues below stage 0 into the flake lock; fiat
retreats to the toolchain/physics boundary). Its one irreducible property is
**pre-existence**: something must already be running before anything can be
derived at runtime. Everything it contains is a node; nothing about it is
special except *when* it was derived. Per the freezer doctrine it carries its
source (boot graph + native manifest) for unfreezing/regeneration.

Capability manifest (what the frozen graph must contain nodes for):

1. header decode — the one deliberately non-self-describing layer
   (multiformats convention, compiled in);
2. parse (bytes-of-kind-graph → instances);
3. registry lookup (name → native; populated by the CMake-generated
   per-target registration TU — loud link failure, readable manifest);
4. naive plan (flat; links → pointers, topological order);
5. naive tick (free-running; enough for the control-rate engine graph);
6. slot swap+migrate (the exec/spawn primitive);
7. dataset payload (kind + data + provenance reference — graph is the first
   kind) with primitive query/transform natives;
8. naive resolver (hash → bytes in the local object directory; independently
   invokable forever — the debugger of last resort, kept native by policy);
9. opaque platform-context stash (only platform natives read it).

**Invariant: stage-0 logic never branches on platform.** Per-platform boot
graphs and per-target registries are acceptable; entry symbols (main /
android_main / emscripten) are <10-line trampolines handing the stash to the
same bootloader. NOT in stage 0: HTTP, files-as-API, GL/XR/audio, store,
selection logic, region rules — all packages.

**Boot sequence.** Trampoline → stage 0 ticks the embedded boot graph → spawns
the engine graph into a slot and **parks** as resident fallback (restart
policy; the one tower level that cannot be edited away) → engine graph
observes the environment, splices available capability packages (ordinary
edits into its fan-ins), receives app graphs from instruction sources
(cli-args stash, http node, ws/peer links, embedded constant), compiles,
realizes. PeerCore dissolves: registry → stage 0; swap/queues → slot
machinery; HTTP, mdns, values/probe, screenshots → package nodes. Frozen
programs bypass the bootloader by design.

## Requirements

**SZ-1 (platform invariance).** One stage-0 compilation unit set, zero
platform conditionals; per-target variance only in trampoline, registration
TU, and embedded boot graph.
- SZ-1.1: grep gate in CI: no `#ifdef` / platform branch in stage-0 sources.
- SZ-1.2: the same stage-0 object code (per-arch) links into host, Quest, and
  web targets.

**SZ-2 (generated registration).** The per-target TU is generated from the
build graph; adding a node type is one file + build entry; omission is a link
error naming the symbol.
- SZ-2.1: deleting a node type's object from the target breaks the link with
  a readable message (not a runtime lookup miss).
- SZ-2.2: the three hand-maintained lists (host 106 / quest 92 / web 41
  register calls) are gone; the palette equals the generated manifest.

**SZ-3 (naive resolver).** hash → bytes with verification; no dependency on
any package; callable from a debug shell even when stage 1 is wedged.
- SZ-3.1: with the store package deliberately broken, stage 0 still resolves
  and boots the embedded boot graph and can load a graph by hash from the
  object directory.

**SZ-4 (frozen-with-provenance).** The stage-0 artifact embeds (or links by
hash to) its boot graph source and native manifest; a tool regenerates the
frozen kernel from them via the Nix derivation.
- SZ-4.1: `unfreeze(stage0-binary)` recovers boot graph JSON + manifest;
  `nix build` from them reproduces a bit-identical kernel (or
  provenance-equal, where toolchain nondeterminism is documented).

**SZ-5 (spawn-and-park).** Stage 0 remains resident; engine-graph crash
triggers restart per policy; stage 0 itself is not editable at runtime.
- SZ-5.1: kill the stage-1 slot 100×; stage 0 restarts it each time; device
  audio glitches are bounded to the swap window.
- SZ-5.2: an edit addressed at stage 0's own graph is rejected with a clear
  error (the fallback cannot be edited away).

**SZ-6 (trampolines).** Each platform entry ≤10 lines: gather stash, call
bootloader.
- SZ-6.1: line-count gate in CI on the three entry files.

**SZ-7 (boot without store).** A peer with an empty object directory boots to
a running engine graph from embedded data alone.
- SZ-7.1: fresh-install boot on all three targets reaches "engine graph
  ticking" with no network and no prior state.

## Worked example (test seed)

Cold-boot trace on host: trampoline (≤10 lines) → stage 0 ticks boot graph →
spawn engine slot → engine splices audio package (observes device) → receives
hello-cosine via cli-args stash → compiles → realizes → sound. Assert order of
phases via probe log; then SZ-5.1's crash-restart loop while hello-cosine
re-arrives from the embedded instruction source.
