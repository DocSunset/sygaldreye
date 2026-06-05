# hrtf_spatializer

Converts a mono audio source at a 3D world position into a stereo binaural signal using the spherical head model.

## Ports

**Inputs**
- `process(mono_in, stereo_out, frames)` — audio callback, sums into stereo_out

**Outputs**
- stereo interleaved PCM (summed into caller's buffer)

**Sources**
- `set_position(direction, distance_m)` — called from scene thread

**Temporal couplings**
- `set_position` may be called concurrently with `process`; triple-buffer ensures no mutex in audio path

## Requirements

- ITD via Woodworth formula; fractional delay with linear interpolation (≤64-sample circular buffer)
- ILD via low-shelf on contralateral channel; gain proportional to `sin(azimuth)`
- Distance attenuation: `1 / max(distance, 0.1)`
- Air absorption: biquad LP, cutoff decreasing from 20 kHz at 0 m to 2 kHz at 20 m
- Elevation pinna notch: notch filter, centre 8–12 kHz mapped by elevation
- No allocation in `process()`

## Allowed dependencies

- `biquad_filter`
- Eigen (math only)

## Owning package

`scene` (audio sub-system)
