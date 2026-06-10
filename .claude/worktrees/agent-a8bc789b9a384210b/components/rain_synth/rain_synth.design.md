# rain_synth

Poisson drop-pool rain synthesizer. Each impact is filtered white noise shaped by a fast decay envelope. Arrival times follow a Poisson process parameterized by drops/second.

## Ports

**Inputs**
- `RainParams` via constructor / `set_params`: rate (drops/s), drop LP cutoff, decay time, sample rate.

**Outputs**
- `fill(float* out, int frames)`: mono interleaved PCM samples.

**Temporal couplings**
- `set_params` may be called between `fill` calls from the audio thread; no lock — caller must ensure no concurrent access.

## Requirements

- No allocation in `fill()`.
- Up to 64 simultaneous drops; excess arrivals are silently dropped.
- Audibly distinguishable drizzle (~20 drops/s) vs. downpour (~2000 drops/s).

## Allowed dependencies

- `synth_core` (`synth::Adsr`, `synth::white_noise`)
- `biquad_filter` (`synth::BiquadFilter`, `synth::low_pass`)
- `audio_scene` (`MonoSynth`)

## Owning package

`scene`
