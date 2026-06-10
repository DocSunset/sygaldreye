# atmos_synth

Synthesises wind and atmospheric ambience as a `MonoSynth`.

## Ports

- **Inputs**: `AtmosParams` via `set_params` (thread-safe)
- **Outputs**: mono PCM via `fill(float*, int)`
- **Sources**: none
- **Destinations**: `MonoSynth` consumer (e.g. `AudioScene`)
- **Temporal couplings**: none
- **Intended seams**: `MonoSynth` virtual interface allows substitution in tests

## Technique

White noise passes through two parallel band-pass filters:

1. Wind band — centre ~`base_freq`, Q 2, frequency slowly modulated by a 0.2 Hz sine LFO (±50% depth).
2. Whistle layer — centre `base_freq * brightness * 4`, Q 8, same frequency modulation; scaled by `brightness`.

A 0.05 Hz amplitude LFO (unipolar sine, range 0–1) multiplies the summed output, creating gusting. Final amplitude scales linearly with `wind_speed`; `wind_speed=0` produces silence.

Filter coefficients are recomputed each sample to follow LFO-driven centre frequency. No heap allocation in `fill`.

## Requirements

- `fill` must be allocation-free
- `set_params` must be safe to call from a thread other than the audio thread

## Allowed dependencies

- `synth_core`
- `biquad_filter`
- `audio_scene`

## Owning package

`scene` (audio synthesis sub-layer)
