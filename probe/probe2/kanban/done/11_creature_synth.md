# creature_synth component

Synthesises biological creature sounds: cricket chirps, bird song, insect buzzing, frog calls. Implements `MonoSynth`. All are rhythmic FM patterns; no recorded samples.

## Approach

- **Cricket chirp**: a train of short FM bursts. Each burst: a carrier sine at ~4–5 kHz, modulated by a carrier-frequency sine at a modulation index of ~2, shaped by a fast ADSR (~20ms attack, ~40ms decay). Bursts repeat at a species rate (~15–30/s in summer) gated by a slower chirp-group envelope (~3 chirps per group, ~1 group/s for field crickets).
- **Bird song**: an FM oscillator pair whose carrier frequency follows a pitch contour — a `std::array` of target pitches with linear interpolation between them, driven by a slow phrase timer. Phrase structure (notes, rests, repetitions) is parameterised. A second harmonic partial at 2× carrier adds fullness.
- **Insect buzz**: FM at audio rate with modulation index ~3–5 gives the harsh buzz character. Amplitude modulated at wing-beat rate (~200–600 Hz for bees).
- `creature_synth.hpp` declares:
  ```cpp
  enum class CreatureKind { Cricket, Bird, Insect };

  struct CreatureParams {
      CreatureKind kind         = CreatureKind::Cricket;
      float carrier_freq        = 4500.0f;
      float fm_index            = 2.0f;
      float chirp_rate          = 20.0f;  // bursts per second (Cricket/Insect)
      std::array<float, 8> pitch_contour; // Hz, for Bird
      float phrase_duration_s   = 2.0f;   // for Bird
      float sample_rate         = 48000.0f;
  };

  class CreatureSynth : public MonoSynth {
  public:
      explicit CreatureSynth(CreatureParams const& = {});
      void set_params(CreatureParams const&);
      void fill(float* out, int frames) override;
  };
  ```
- `creature_synth.test.cpp`: verify Cricket output is silent between chirp groups; verify Bird output has non-zero pitch variation across a phrase (carrier frequency changes).

## Acceptance

- Cricket, bird, and insect presets are recognisable (on-device)
- No allocation in `fill()`
- `creature_synth.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- `synth_core` (item 01)
- `biquad_filter` (item 02)
