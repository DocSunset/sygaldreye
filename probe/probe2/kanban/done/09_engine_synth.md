# engine_synth component

Synthesises mechanical engines, electric motors, HVAC systems, and hovercraft — any machine with a dominant rotational frequency and harmonic structure. Implements `MonoSynth`. RPM is a runtime parameter so engines can accelerate, idle, and rev.

## Approach

- **Additive engine model**: a `Phasor` at the fundamental frequency (`rpm / 60` Hz for a single-cylinder engine, `rpm / 60 * cylinders / 2` for a four-stroke multi-cylinder). Harmonic partials at integer multiples with amplitude rolloff. A small amount of FM (modulation index ~0.2) adds mechanical roughness.
- **Noise layer**: band-pass noise centred at ~2–3× fundamental for mechanical texture (friction, air turbulence).
- **HVAC variant**: omit harmonics; use narrow band-pass noise at the blower frequency; add a 50/60 Hz hum via sine oscillator.
- `engine_synth.hpp` declares:
  ```cpp
  struct EngineParams {
      float rpm            = 800.0f;
      int   cylinders      = 4;      // affects harmonic pattern
      float roughness      = 0.3f;   // FM modulation depth
      float load           = 0.5f;   // 0=idle, 1=full load; affects harmonic balance
      float sample_rate    = 48000.0f;
  };

  class EngineSynth : public MonoSynth {
  public:
      static constexpr int k_max_harmonics = 8;
      explicit EngineSynth(EngineParams const& = {});
      void set_params(EngineParams const&); // rpm changes take effect smoothly
      void fill(float* out, int frames) override;
  };
  ```
- RPM changes are smoothed over ~50ms to avoid click artefacts (interpolate phasor frequency update).
- `engine_synth.test.cpp`: verify output frequency (via zero-crossing count) matches `rpm/60` within 5%; verify increasing `rpm` raises pitch.

## Acceptance

- A car engine idling at 800 RPM and revving to 4000 RPM sounds convincingly mechanical (on-device)
- RPM ramp produces a smooth pitch rise with no audible clicks
- No allocation in `fill()`
- `engine_synth.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `synth_core` (item 01)
- `biquad_filter` (item 02)
