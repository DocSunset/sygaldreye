# hrtf_spatializer component

Converts a mono audio source at a 3D world position into a stereo binaural signal. Uses the spherical head model for ITD (inter-aural time delay) and ILD (inter-aural level difference), plus a pair of high-shelf filters to approximate pinna coloration. No external HRTF database required.

## Approach

- `hrtf_spatializer.hpp` declares:
  ```cpp
  struct SpatializerParams {
      float head_radius_m  = 0.0875f; // ~8.75 cm, average adult
      float sample_rate    = 48000.0f;
  };

  // One instance per audio source. Processes one block at a time.
  class HrtfSpatializer {
  public:
      explicit HrtfSpatializer(SpatializerParams const& = {});

      // Call from the scene thread (not audio thread) to update position.
      // direction: unit vector from listener to source (listener-space).
      // distance_m: source distance; drives amplitude and air absorption.
      void set_position(Eigen::Vector3f direction, float distance_m);

      // Call from the audio callback. Writes `frames` stereo samples
      // (interleaved L/R) into `out`, summing into whatever is already there.
      void process(float const* mono_in, float* stereo_out, int frames);
  };
  ```
- **ITD**: `delay = head_radius / c * (azimuth + sin(azimuth))` (Woodworth formula). Implemented as a fractional-sample delay line (linear interpolation) on the contralateral channel.
- **ILD**: frequency-dependent level difference via a first-order low-shelf on the contralateral channel; shelf gain is a function of azimuth.
- **Distance**: amplitude scales as `1 / max(distance, 0.1)`. A low-pass filter (`biquad_filter`) with cutoff decreasing with distance models air absorption.
- **Elevation**: approximate pinna notch via a notch filter on both channels; centre frequency shifts with elevation.
- Position is exchanged between scene and audio threads via a small lock-free triple-buffer (three copies of a `PositionParams` struct; atomic index swap).
- `hrtf_spatializer.test.cpp`: a source directly to the left produces positive ITD on the right channel; a source directly ahead produces equal amplitude in both channels.

## Acceptance

- A mono tone placed 90° to the left is audibly localised left in-headset
- A source 10 m away is quieter than the same source at 1 m
- No dynamic allocation in `process()`
- `hrtf_spatializer.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `biquad_filter` (item 02)
- `synth_core` (item 01) — used in tests
