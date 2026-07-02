# synth_core

Stateless oscillator functions, xorshift noise, a phase accumulator, an ADSR envelope, an LFO, per-sample DSP kernels (`kernels.hpp`), and the generator block shell (`block_shell.hpp`).

## Ports

**Inputs**
- `phase` argument to oscillator functions — caller owns the phase value
- `state` reference to `white_noise` — caller owns and seeds the PRNG state
- `Phasor::freq`, `Phasor::sample_rate` — set before calling `tick()`
- `Adsr` fields (including `sample_rate`) — set before calling `gate_on()` / `tick()`
- `Lfo::depth`, `Lfo::phasor` — set before calling `tick()`
- Kernel params (`kernels.hpp`) — plain members or `tick()` arguments; `DelayLine::prepare(len)` sizes the line outside `tick()`
- `Gen::generate(t, body)` — caller supplies absolute time; the shell converts to elapsed frames

**Outputs**
- Return values of all `tick()` and oscillator functions — audio-rate scalars in [-1, 1] (ADSR in [0, 1])
- `Gen::generate` — a mono `AudioBuffer` view over the shell-owned buffer

**Sources / Destinations / Temporal couplings**
- None beyond the allowed dependency below.

**Intended seams**
- All structs are plain aggregates; callers embed them anywhere. One kernel instance per channel is the lifting unit (ugens `Lift`).

## Requirements

- All oscillator outputs bounded to [-1, 1].
- ADSR stages: Attack → Decay → Sustain on `gate_on()`; Release → Done on `gate_off()`.
- `white_noise` uses xorshift32; output in [-1, 1].
- No allocation per sample: `tick()` never allocates. Allocation happens only at prepare/first-touch (`DelayLine::prepare`, `Gen`'s buffer growth), idempotently. The residual (first touch can land in the audio callback) is tracked in `kanban/backlog/audio_region_followups.md`.
- No absolute-time state in kernels — time arrives as per-sample increments (epoch-robustness; see audio_region_followups.md).
- No audio framework dependency.

## Allowed dependencies

- `sygaldry_endpoints` (block_shell's `AudioBuffer`)

## Owning package

`scene`
