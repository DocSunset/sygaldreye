# Kernel extraction (overnight 3) — working plan

Goal: every audio ugen's DSP is a per-sample KERNEL — a plain struct,
state + `tick()`, no buffers, no node machinery — and the node is a thin
block shell that loops the kernel per channel. The freezer composes
kernels; conformability lifts them; ugens are born endpoints-v6 here
(deferred phase C).

## Kernel contract (ratified pieces)
- Per-sample tick; params are plain members written before the loop.
- NO absolute-time state (the metro epoch bug class). Time arrives as
  dt or per-sample increments. Gen::frames-style dt clamping stays in
  the SHELL, never the kernel.
- One kernel instance per channel = the lifting/conformability unit.
- Kernels live in components/synth_core (it already holds Adsr,
  BiquadFilter, shapers — this normalizes the rest into the same place).
- POD-friendly: no allocation in tick(); delay lines own their buffers
  but size them in prepare(rate, max), not per tick.

## Inventory (ugens.hpp + friends)
| node        | kernel today?                  | to extract               |
|-------------|--------------------------------|--------------------------|
| osc         | synth shapers (sine/saw/...)   | PhaseOsc{phase, tick(dphi)} |
| noise       | inline white→pink (Kellet)     | PinkNoise{state, tick()} |
| perc/adsr   | synth::Adsr ✓                  | shell only               |
| biquad      | synth::BiquadFilter ✓          | shell only               |
| vca         | multiply                       | trivial, inline ok       |
| mix         | sum                            | trivial, inline ok       |
| delay       | inline ring                    | DelayLine{buf, tick(x, samples)} |
| shaper      | inline tanh                    | use synth shaper fn      |
| sample_hold | inline                         | SampleHold{held, tick(x, trig)} |
| slew        | inline                         | Slew{y, tick(x, up, dn)} |
| metro       | gate from t (epoch-fixed)      | keep in shell (time node)|
| grain_cloud | inline voices                  | GrainVoice{env,phase,tick()} |
| spatialize  | HrtfSpatializer ✓ (per-block)  | leave (Steam Audio v2 pending) |

## Steps
1. synth_core: add PhaseOsc, PinkNoise, DelayLine, SampleHold, Slew,
   GrainVoice kernels (move logic out of ugens.hpp verbatim where
   possible). Unit-test kernels directly (synth_core test).
2. ugens.hpp: shells loop kernels; per-channel kernel vectors replace
   per-channel ad-hoc state. SAME block semantics — ugens.test must
   pass unchanged.
3. v6 migration of ugens + osc_node + dac_node in the same pass:
   endpoints struct; audio in<AudioBuffer>, params normalled_in,
   triggers event_in. Presets (assets/graphs/*_g.json params) keep
   working via set_*/deserialize fallback writers.
4. Verify: host tests, android build, device replay (playground levels
   + FPS probes), commit.

## Status
- [x] plan written
- [x] step 1 (kernels + tests: PinkNoise, DelayLine, SampleHold, Slew, GrainVoice in synth_core/kernels.hpp; 5 unit tests)
- [x] step 2 (noise/delay/sample_hold/slew/grain_cloud shells loop kernels; osc_node already kernel-shaped; ugens.test green)
- [>] step 3 (v6 migration of ugens) DEFERRED: legacy producers
      (lfo, controllers) into v6 in<T> stream inputs are dead edges —
      needs the whole-cluster move, scheduled with phase C proper.
- [x] step 4 (host tests green, android clean, device replay alive — NOTE: hit the swap-poison bug en route, see kanban/backlog/block_swap_poison.md; kernels exonerated)
