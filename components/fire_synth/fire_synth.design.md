# fire_synth

Synthesises a crackling fire sound from three additive noise layers.

## Ports

**Inputs**
- `fill(out, frames)`: request for `frames` samples of mono audio written to `out`

**Outputs**
- PCM samples written to caller-supplied buffer

**Sources**
- `FireParams` via `set_params()`: intensity, crackle_rate, sample_rate — may be called from any thread

**Destinations**
- Caller mixes returned samples into a scene bus

**Temporal couplings**
- `fill()` must not be called concurrently with itself; single-consumer audio thread assumed

**Intended seams**
- `MonoSynth` base allows drop-in substitution in `AudioScene`

## Requirements

- No dynamic allocation in `fill()`
- Three layers: roar (band-pass, LFO-modulated centre and amplitude), crackle (Poisson transient pool, high-passed), hiss (high-pass, low amplitude)
- `intensity=0` produces silence
- `set_params` is thread-safe (atomic store/load)

## Allowed dependencies

- `synth_core`
- `biquad_filter`
- `audio_scene`

## Owning package

`scene`
