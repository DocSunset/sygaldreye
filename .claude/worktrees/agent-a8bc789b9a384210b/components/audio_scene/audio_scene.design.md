# audio_scene

Mixes a fixed set of spatially-positioned mono synth sources into a stereo output buffer each audio callback.

## Ports

**Inputs**
- `mix(stereo_out, frames)` — called by the audio callback thread; writes interleaved stereo float samples

**Outputs**
- None (writes into caller-supplied buffer)

**Sources**
- `MonoSynth::fill` — per-source sample generator, owned externally

**Destinations**
- `HrtfSpatializer::process` — spatialized stereo mix-in

**Temporal couplings**
- `set_listener` / `update_source` are called on the scene thread (72 Hz); `mix` runs on the audio thread (~200 Hz). `HrtfSpatializer` provides lock-free triple-buffer isolation for position updates.

**Intended seams**
- `MonoSynth` abstract base allows injecting any mono generator (synths, test doubles)

## Requirements

- Up to `k_max_sources = 16` simultaneous sources
- No heap allocation in `mix()`; mono scratch buffer stack-allocated (frames ≤ 256)
- Removing a source takes effect within one callback block
- Sources mix additively; output cleared before mix

## Allowed dependencies

- `hrtf_spatializer`
- Eigen

## Owning package

`scene`
