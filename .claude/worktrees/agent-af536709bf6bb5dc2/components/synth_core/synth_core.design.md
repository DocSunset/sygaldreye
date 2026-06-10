# synth_core

Stateless oscillator functions, xorshift noise, a phase accumulator, an ADSR envelope, and an LFO.

## Ports

**Inputs**
- `phase` argument to oscillator functions — caller owns the phase value
- `state` reference to `white_noise` — caller owns and seeds the PRNG state
- `Phasor::freq`, `Phasor::sample_rate` — set before calling `tick()`
- `Adsr` fields — set before calling `gate_on()` / `tick()`
- `Lfo::depth`, `Lfo::phasor` — set before calling `tick()`

**Outputs**
- Return values of all `tick()` and oscillator functions — audio-rate scalars in [-1, 1] (ADSR in [0, 1])

**Sources / Destinations / Temporal couplings**
- None. No dependency on any other component.

**Intended seams**
- All structs are plain aggregates; callers may embed them anywhere without allocation.

## Requirements

- All oscillator outputs bounded to [-1, 1].
- ADSR stages: Attack → Decay → Sustain on `gate_on()`; Release → Done on `gate_off()`.
- `white_noise` uses xorshift32; output in [-1, 1].
- No dynamic allocation.
- No audio framework dependency.

## Allowed dependencies

None.

## Owning package

`scene`
