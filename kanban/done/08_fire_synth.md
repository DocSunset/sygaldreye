# fire_synth component

Synthesises a crackling fire: a continuous roar of filtered noise overlaid with irregular crackle transients and a slow thermal flicker. Implements `MonoSynth`. Covers: campfire, torch, fireplace, ember glow, wildfire roar.

## Approach

- Three additive layers:
  1. **Roar**: white noise → band-pass (~150–800 Hz) with slowly drifting centre frequency (LFO + noise), amplitude modulated by a slow LFO to suggest breathing.
  2. **Crackle**: random transient spikes (white noise × very fast decay envelope) triggered at irregular Poisson intervals (~20–80/s for a campfire). Each crackle is high-passed to give a sharp snap character.
  3. **Hiss**: white noise → high-pass (~2 kHz), low amplitude, gives the fire its air and breath.
- `fire_synth.hpp` declares:
  ```cpp
  struct FireParams {
      float intensity    = 1.0f; // 0=embers, 1=campfire, 3=bonfire
      float crackle_rate = 40.0f; // transients per second
      float sample_rate  = 48000.0f;
  };

  class FireSynth : public MonoSynth {
  public:
      static constexpr int k_max_crackles = 16;
      explicit FireSynth(FireParams const& = {});
      void set_params(FireParams const&);
      void fill(float* out, int frames) override;
  };
  ```
- `fire_synth.test.cpp`: verify `intensity=0` produces near-zero output; verify output RMS is monotonically higher at greater intensities.

## Acceptance

- A campfire and a bonfire are audibly distinct; crackle timing feels organic (on-device)
- No allocation in `fill()`
- `fire_synth.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `synth_core` (item 01)
- `biquad_filter` (item 02)
