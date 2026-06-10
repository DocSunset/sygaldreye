# chime_synth

Modal synthesis of struck resonant objects: wind chimes, bells, metallic impacts.

## Ports

- **Inputs**: `set_params(ChimeParams)` — reconfigure modes, decay times, sample rate
- **Inputs**: `strike(gain)` — trigger a strike event (thread-safe)
- **Outputs**: `fill(float*, int)` — write mono PCM samples to caller-provided buffer
- **Sources**: none
- **Destinations**: `MonoSynth::fill` consumed by `AudioScene`
- **Temporal couplings**: `strike()` may be called from any thread; effect is applied at the start of the next `fill()` block
- **Intended seams**: `ChimeParams` is plain data; presets are free functions

## Requirements

- Modal synthesis: each mode is `A_i * sin(2π f_i t) * exp(-t / τ_i)`
- Up to 8 simultaneous modes; active count set by `mode_count`
- `strike()` is thread-safe via `std::atomic<float> pending_strike_gain_`
- Auto-strike: when `strike_rate > 0`, Poisson accumulator triggers strikes in `fill()`
- No dynamic allocation in `fill()`
- `-Wall -Wextra` clean

## Allowed dependencies

- `synth_core` — `synth::sine`, `synth::Phasor`
- `audio_scene` — `MonoSynth` base class

## Owning package

`scene`
