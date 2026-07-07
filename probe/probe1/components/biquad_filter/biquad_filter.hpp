#pragma once
namespace synth {

struct BiquadCoeffs {
    float b0, b1, b2;
    float a1, a2;
};

BiquadCoeffs low_pass  (float freq, float q, float sample_rate);
BiquadCoeffs high_pass (float freq, float q, float sample_rate);
BiquadCoeffs band_pass (float freq, float q, float sample_rate);
BiquadCoeffs notch     (float freq, float q, float sample_rate);
BiquadCoeffs low_shelf (float freq, float gain_db, float sample_rate);
BiquadCoeffs high_shelf(float freq, float gain_db, float sample_rate);

struct BiquadFilter {
    BiquadCoeffs coeffs{};
    float s1=0, s2=0;
    float tick(float in);
    void  set_coeffs(BiquadCoeffs const&);
};

} // namespace synth
