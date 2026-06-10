# engine_synth

Synthesises rotational machinery (engines, motors) as a `MonoSynth`.

## Owning package

`scene`

## Allowed dependencies

`synth_core`, `biquad_filter`, `audio_scene`

## Ports

**Inputs**
- `EngineParams` on construction and via `set_params`

**Outputs**
- `fill(float*, int)` — mono PCM samples

**Temporal couplings**
- `fill` must be called from a single audio thread; `set_params` may be called from any thread.

## Requirements

- Fundamental frequency: `rpm / 60 * cylinders / 2` Hz (four-stroke formula)
- `k_max_harmonics` additive partials with `1/(i+1)` amplitude rolloff, scaled by `load`
- FM roughness via modulation phasor at the fundamental, index = `roughness`
- Band-pass noise layer centred at `fund_hz * 2.5`, Q=2, amplitude 0.1
- RPM changes smoothed over ~50 ms (single-pole IIR per sample)
- No allocation in `fill()`
- Thread-safe parameter hand-off via `std::atomic<EngineParams>`
