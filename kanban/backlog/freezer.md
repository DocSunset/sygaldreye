# The freezer: graph JSON → optimized, portable, typed C++ (ratified 2026-06-12)

> 2026-07-03 amendments: freeze is a DERIVATION (recipe provenance = the
> "carries source JSON" doctrine, now free); freeze-as-a-service = capability
> placement fallthrough; per-sample feedback is now the INTERPRETER's default
> too (ADR-013) — the freezer is a pure optimizer, never a semantics change;
> "backend of compilation" framing proposed (see architecture/05). Trigger
> status: v6 ✓, spans/conformability ✓, values map dead ✗.

Revives phase 9 (freeze_graph.py / compile_frozen.py / ABI provenance
fields) with four ratified decisions from the design conversation.

## 1. Codegen, not constexpr (decision recorded)
A runtime JSON→.so node must invoke a compiler subprocess regardless —
constexpr resolution only ever happens inside some TU, so the
"compile-time graph" and "generated fully-typed source" converge to the
same machine code at the point of use. The generator emits plain typed
C++ (`OscNode n0; n1.inputs.audio.value = n0.outputs.audio.value; …`):
identical optimizer visibility (devirtualized process, no values-map
hashing, fused loops, SIMD), sane compile times, readable errors, and no
compile-time type registry to fight the plugin system.

## 2. Per-sample kernels (the gen~/Faust layer)
- Every audio ugen = a per-sample KERNEL (pure value-type `tick(state,
  in)`, per-channel state, params read from a struct latched per block)
  + a thin node SHELL (ports/buffers/channels) that loops it. synth_core
  already has most kernels; the refactor is extraction + discipline.
- INTERPRETED executor stays block-granular (shells looping kernels —
  today's performance). FROZEN graphs fuse kernels per-sample: feedback
  edges become loop-carried variables = single-sample z⁻¹ for free
  (Karplus-Strong, feedback FM, filters in loops). Feedforward chains
  fuse per-sample too; a kernel may declare a block override (FFT-shaped
  work) which breaks fusion at that node.
- Kernel purity tiers: "freestanding" = cmath only, no OS, no alloc
  after init (all current ugens qualify; required for tier-1 targets).

## 3. Portable-component-first output (the inversion)
Primary artifact: a plain C++ class — POD endpoint structs (scalar→
float, vec3→float[3], audio→float*+count, bang→bool, text→const char*),
simple tick(), dependency closure = kernel headers only. NO Eigen/std::
string/ABI in the public surface. Packagings of that class:
  (a) plugin .so (descriptor adapter; hot-reload is the deployment path)
  (b) firmware scaffold (later; see tiers)
  (c) possible Sygaldry-component adapter — the frozen artifact IS
      structurally a Sygaldry component (named endpoints + tick): the
      VR environment authors embedded instruments.
Freeze-time BAKING: channel counts, sample rate, block size, subgraph
composition are wire-determined and known → static arrays, no post-init
allocation. Authoring stays dynamic (Pd-style); artifacts are static.

## 4. General freezing with target tiers (not audio-only)
Any graph freezes; the generator computes WHERE it runs from per-node
provenance/dependency closure:
  tier 1 freestanding (kernels/math only) → MCUs, anywhere C++ runs
  tier 2 platform-lib (GL nodes, dac stream) → GLES/audio hosts
  tier 3 host-bound (tmux/HTTP nodes) → desktop peers
Editor could surface a subgraph's tier (design pressure toward portable
graphs). Frozen artifacts CARRY THEIR SOURCE JSON as provenance —
unfreeze recovers the editable subgraph (guiding star survives
optimization).

## The node + the service
- `freeze` is a NODE (worker region; clang/NDK subprocess via the
  compile_node.py machinery). Quest has no toolchain and needs none:
  freeze runs on the linux peer and is ADVERTISED over the bridge —
  compilation-as-a-service; a quest graph wires JSON to the remote
  freeze node, the .so comes back via POST /plugins. Browser: emscripten
  side-module packaging, own rung.

## Build order
1. Kernel extraction refactor (ugens → kernels + shells; tests stay
   green; freestanding discipline stated per kernel).
2. Generator v2 (freeze_graph.py revival or C++ rewrite): portable class
   emitter for block-region subgraphs, per-sample fusion + feedback
   variables; bake channels/rates.
3. Plugin packager + `freeze` node on worker region; A/B chime
   interpreted-vs-frozen: spectrogram diff ≈ 0, block-time stat shows
   the speedup; hot-reload swap live.
4. Freestanding proof: arm-none-eabi-g++ -ffreestanding compile+link of
   frozen chime, no OS symbols (long before real hardware).
5. Freeze-as-a-service over the bridge (quest target via NDK).
6. General (frame-region) freezing + tier reporting; firmware scaffold +
   board bring-up (target TBD — Daisy/Teensy/ESP32 class) when Travis
   picks hardware.

## Hardware note (to discuss — Travis, 2026-06-12)
Likely first firmware target: Raspberry Pi Pico 2 W (RP2350) via the
Pico SDK — pencilled in for build-order rung 6, NOT yet ratified.
Discuss before bring-up: Pico SDK packaging in nix, RP2350 FPU/DSP
budget vs the kernel set, I2S/PWM audio out, and whether the W's wifi
makes the frozen artifact a sygaldreye PEER (advertised nodes from a
microcontroller!).

## POSTPONED (ratified 2026-06-12)

Deferred until the runtime execution model settles. Trigger condition:
endpoints v6 migration complete + values map dead + spans/conformability
landed. Rationale (Travis): maintaining runtime + codegen execution
models simultaneously doubles every executor bug while the first model
is still moving; codegen/toolchain/provenance territory (the C++-as-a-
language frontier) deserves a settled foundation. The per-sample kernel
discipline (synth_core/kernels.hpp) is preserved specifically to keep
this pickable-up. Embedded targets (Pi Pico 2 W) ride this trigger.
