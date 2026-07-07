# atmos_synth component

Synthesises wind, ambient atmosphere, and distant environmental drones. Implements `MonoSynth`. Covers: open-field wind, forest ambience, cave hum, space station HVAC, desert silence with distant heat shimmer.

## Approach

- Technique: white noise → band-pass filter whose centre frequency and Q drift slowly via two LFOs driven by different `noise` seeds. A slow amplitude LFO adds gusting.
- `atmos_synth.hpp` declares:
  ```cpp
  struct AtmosParams {
      float wind_speed     = 1.0f;  // 0=still, 1=moderate breeze, 3=gale
      float base_freq      = 300.0f; // spectral centre of the wind band
      float brightness     = 0.5f;  // 0=muffled, 1=sharp/whistling
      float sample_rate    = 48000.0f;
  };

  class AtmosSynth : public MonoSynth {
  public:
      explicit AtmosSynth(AtmosParams const& = {});
      void set_params(AtmosParams const&); // thread-safe via atomic load
      void fill(float* out, int frames) override;
  };
  ```
- Implementation: noise → band-pass (centre modulated by slow LFO) → amplitude envelope (slow LFO, depth proportional to `wind_speed`). A second band-pass in parallel at a higher frequency adds a whistle layer when `brightness` is high.
- `atmos_synth.test.cpp`: verify `fill` with `wind_speed=0` produces near-zero output; verify output amplitude increases with `wind_speed`.

## Acceptance

- Audibly distinguishable wind character at low, medium, and high `wind_speed` (on-device verification)
- No allocation in `fill()`
- `atmos_synth.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `synth_core` (item 01)
- `biquad_filter` (item 02)
- `noise` (proc_env item 01) — seeds LFO variation
