# rain_synth component

Synthesises rain from individual drop impacts, each a short burst of filtered noise, arriving at Poisson-distributed times proportional to rainfall rate. Implements `MonoSynth`. Covers: light drizzle, heavy downpour, hail, rain on leaves vs. rain on a tin roof.

## Approach

- Each raindrop is a brief white noise burst shaped by a fast decay envelope (attack ~0.5ms, decay ~5–15ms), low-passed at a surface-dependent cutoff (high for hard surfaces, lower for foliage).
- A Poisson arrival process: per sample, emit a new drop with probability `dt * rate` where `rate` is drops/second. Up to N drops active simultaneously in a fixed pool.
- `rain_synth.hpp` declares:
  ```cpp
  struct RainParams {
      float rate          = 200.0f; // drops per second; ~20=drizzle, ~2000=downpour
      float drop_freq     = 4000.0f; // low-pass cutoff of each drop impact
      float decay_s       = 0.01f;   // envelope decay time in seconds
      float sample_rate   = 48000.0f;
  };

  class RainSynth : public MonoSynth {
  public:
      static constexpr int k_max_drops = 64;
      explicit RainSynth(RainParams const& = {});
      void set_params(RainParams const&);
      void fill(float* out, int frames) override;
  };
  ```
- Drop pool: fixed array of `{Adsr env, BiquadFilter lp, uint32_t noise_state, bool active}`. Inactive slots are recycled for new drops.
- `rain_synth.test.cpp`: verify `fill` with `rate=0` is silent; verify increasing `rate` increases RMS output energy.

## Acceptance

- Light and heavy rain are audibly distinct and convincing (on-device verification)
- No allocation in `fill()`
- `rain_synth.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `synth_core` (item 01)
- `biquad_filter` (item 02)
