# biquad_filter component

A second-order IIR (biquad) filter with configurable type and coefficients. The workhorse of all audio DSP in this stack — nearly every sound generator is a chain of these. Supports low-pass, high-pass, band-pass, notch, and low/high shelf types.

## Approach

- `biquad_filter.hpp` declares:
  ```cpp
  struct BiquadCoeffs {
      float b0, b1, b2; // feedforward
      float a1, a2;     // feedback (a0 normalised to 1)
  };

  // Coefficient factories (Audio EQ Cookbook formulas)
  BiquadCoeffs low_pass (float freq, float q, float sample_rate);
  BiquadCoeffs high_pass(float freq, float q, float sample_rate);
  BiquadCoeffs band_pass(float freq, float q, float sample_rate);
  BiquadCoeffs notch    (float freq, float q, float sample_rate);
  BiquadCoeffs low_shelf(float freq, float gain_db, float sample_rate);
  BiquadCoeffs high_shelf(float freq, float gain_db, float sample_rate);

  struct BiquadFilter {
      BiquadCoeffs coeffs{};
      float x1=0,x2=0,y1=0,y2=0; // delay state
      float tick(float in);
      void  set_coeffs(BiquadCoeffs const&); // can update mid-stream
  };
  ```
- Direct Form II transposed implementation for numerical stability.
- `biquad_filter.test.cpp`: verify low-pass attenuates a signal above its cutoff; verify band-pass passes near its centre frequency; verify DC passes through a high-pass at zero.

## Acceptance

- Host-buildable and host-testable
- Frequency response of each type matches expectation at cutoff (within 3 dB tolerance in tests)
- `biquad_filter.design.md` present; `.cpp` under 100 lines of substantive code

## Dependencies

- None
