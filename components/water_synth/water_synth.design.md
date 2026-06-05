# water_synth

Procedural water synthesizer. Layers filtered noise (flow), slow amplitude modulation (waves), and transient bursts (splashes).

## Ports

- **Input**: `WaterParams` via `set_params()` (thread-safe, atomic)
- **Output**: mono PCM via `fill(float*, int)` — inherits `MonoSynth`

No sources, destinations, or temporal couplings beyond `MonoSynth` contract.

## Intended seam

`MonoSynth` vtable — can be swapped into any `AudioSource`.

## Requirements

- No allocation in `fill()`
- `flow_speed=0` → near-silent output
- `wave_rate>0` → measurable periodic amplitude modulation
- `wave_height` scales both wave modulation depth and splash amplitude
- `brightness` shifts spectral centre and LFO modulation depth

## Layers

1. **Flow**: white noise → band-pass (centre `400 + brightness*800` Hz, Q=2), centre modulated by two slow LFOs (0.07 Hz, 0.13 Hz) with depth `±brightness*200` Hz. Amplitude scaled by `flow_speed`.
2. **Wave**: phasor at `wave_rate` Hz; envelope = `(1-wave_height) + wave_height * 0.5*(1+sin(phase))`. Multiplies flow output.
3. **Splash**: when wave envelope crosses 0.9 upward, activates one of 4 pooled splash slots — white noise → low-pass (3 kHz), exponential decay (τ=80 ms). Scaled by `wave_height`.

## Allowed dependencies

- `synth_core`
- `biquad_filter`
- `audio_scene`

## Owning package

`scene`
