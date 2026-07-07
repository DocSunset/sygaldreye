# chime_synth component

Synthesises struck or bowed resonant objects: wind chimes, bells, metallic impacts, glass, wooden percussion, robot joint clicks. Implements `MonoSynth`. Uses modal synthesis: a sum of exponentially decaying sinusoids at the object's resonant mode frequencies.

## Approach

- Modal synthesis: each mode is `A_i * sin(2π * f_i * t) * exp(-t / τ_i)` where `f_i` are the mode frequencies, `A_i` their relative amplitudes, and `τ_i` their decay time constants. A struck event initialises all modes simultaneously.
- `chime_synth.hpp` declares:
  ```cpp
  struct Mode {
      float freq;       // Hz
      float amplitude;  // relative [0,1]
      float decay_s;    // 60 dB decay time
  };

  struct ChimeParams {
      std::array<Mode, 8> modes;
      int   mode_count   = 4;       // how many of the 8 slots are active
      float strike_rate  = 0.0f;    // auto-strikes per second (0=manual only)
      float strike_gain  = 1.0f;
      float sample_rate  = 48000.0f;
  };

  class ChimeSynth : public MonoSynth {
  public:
      explicit ChimeSynth(ChimeParams const& = {});
      void set_params(ChimeParams const&);
      void strike(float gain = 1.0f); // trigger a strike event (thread-safe)
      void fill(float* out, int frames) override;
  };
  ```
- Preset factories (free functions alongside the class):
  - `wind_chime_params()` — 5 modes with inharmonic ratios (~1 : 2.76 : 5.40 : 8.93 : 13.34), long decays
  - `bell_params()` — 8 modes based on church bell partial ratios, very long decays
  - `robot_click_params()` — 3 short-decay modes, high frequencies, fast strike rate
- `chime_synth.test.cpp`: verify `strike()` produces non-zero output that decays to near-zero within the longest mode's `decay_s`; verify `strike_rate=0` with no manual strikes is silent.

## Acceptance

- Wind chime and bell presets are tonally convincing (on-device)
- Decay times match the specified `decay_s` values within 20% (test-verifiable)
- No allocation in `fill()`
- `chime_synth.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `synth_core` (item 01)
