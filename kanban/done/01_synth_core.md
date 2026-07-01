# synth_core component

The primitive signal generators: oscillators, noise, ADSR envelopes, and LFOs. Everything in the audio stack is built from combinations of these. All operate sample-by-sample (or block-by-block) at a caller-supplied sample rate.

## Approach

- `synth_core.hpp` declares all types in a `synth` namespace:
  ```cpp
  // Stateless per-sample generators
  float sine(float phase);      // phase in [0, 2π)
  float square(float phase);
  float sawtooth(float phase);
  float triangle(float phase);
  float white_noise(uint32_t& state); // xorshift32, caller owns state

  // Phase accumulator (advances phase by 2π * freq / sample_rate per sample)
  struct Phasor {
      float phase     = 0.0f;
      float freq      = 440.0f;
      float sample_rate;
      float tick();  // returns current phase then advances
  };

  // ADSR envelope
  struct Adsr {
      float attack_s, decay_s, sustain_level, release_s;
      float sample_rate;
      enum class Stage { Attack, Decay, Sustain, Release, Done };
      Stage stage = Stage::Done;
      void  gate_on();
      void  gate_off();
      float tick();  // returns amplitude [0, 1]
  };

  // LFO: a Phasor feeding a waveform at sub-audio rate, output in [-1, 1]
  struct Lfo {
      Phasor phasor;
      float  depth = 1.0f;
      float  tick();  // sine LFO
  };
  ```
- `synth_core.cpp`: implement each; no dynamic allocation; no audio-framework dependency.
- `synth_core.test.cpp`: verify sine/square/saw/tri are bounded in [-1,1]; verify ADSR stages progress correctly; verify `white_noise` output is in [-1,1] and not constant.

## Acceptance

- Host-buildable and host-testable (no Android, no GL, no XR)
- All tests pass
- `synth_core.design.md` present; `.cpp` under 100 lines of substantive code
