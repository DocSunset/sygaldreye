# water_synth component

Synthesises flowing and lapping water: rivers, streams, ocean waves, waterfalls, and rain on a surface. Implements `MonoSynth`. Covers a continuum from a babbling brook to a crashing surf.

## Approach

- **Flow layer**: white noise → band-pass filter (centre ~400–1200 Hz) with slowly modulated centre frequency and Q (two independent slow LFOs) — gives the "burbling" quality of moving water.
- **Wave layer**: the flow layer's amplitude is modulated by a low-frequency envelope (~0.1–0.5 Hz) whose shape and rate are parameterised. For ocean waves, the amplitude surges and falls; for a river, it barely moves.
- **Splash transients**: at wave crests, brief filtered noise bursts (similar to `rain_synth` drops but longer, ~50–200ms) add impact.
- `water_synth.hpp` declares:
  ```cpp
  struct WaterParams {
      float flow_speed    = 1.0f;  // 0=still pool, 1=stream, 3=waterfall
      float wave_rate     = 0.2f;  // waves per second (0=river, 0.1–0.3=ocean)
      float wave_height   = 0.5f;  // amplitude of wave modulation [0,1]
      float brightness    = 0.5f;  // spectral brightness of the water noise
      float sample_rate   = 48000.0f;
  };

  class WaterSynth : public MonoSynth {
  public:
      explicit WaterSynth(WaterParams const& = {});
      void set_params(WaterParams const&);
      void fill(float* out, int frames) override;
  };
  ```
- `water_synth.test.cpp`: verify `flow_speed=0` produces near-zero output; verify `wave_rate=0.2` produces periodic amplitude modulation detectable by measuring block-to-block RMS over several seconds.

## Acceptance

- River, ocean, and waterfall presets are audibly distinct (on-device)
- No allocation in `fill()`
- `water_synth.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `synth_core` (item 01)
- `biquad_filter` (item 02)
