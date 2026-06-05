#include "biquad_filter.hpp"
#include <cmath>

namespace synth {

static constexpr float kPi = 3.14159265358979323846f;

namespace {
struct Raw { float b0,b1,b2,a0,a1,a2; };
BiquadCoeffs normalise(Raw r) {
    return {r.b0/r.a0, r.b1/r.a0, r.b2/r.a0, r.a1/r.a0, r.a2/r.a0};
}
}

BiquadCoeffs low_pass(float freq, float q, float sample_rate) {
    float w0    = 2*kPi*freq/sample_rate;
    float alpha = sinf(w0)/(2*q);
    float cosw0 = cosf(w0);
    return normalise({(1-cosw0)/2, 1-cosw0, (1-cosw0)/2,
                      1+alpha, -2*cosw0, 1-alpha});
}

BiquadCoeffs high_pass(float freq, float q, float sample_rate) {
    float w0    = 2*kPi*freq/sample_rate;
    float alpha = sinf(w0)/(2*q);
    float cosw0 = cosf(w0);
    return normalise({(1+cosw0)/2, -(1+cosw0), (1+cosw0)/2,
                      1+alpha, -2*cosw0, 1-alpha});
}

BiquadCoeffs band_pass(float freq, float q, float sample_rate) {
    float w0    = 2*kPi*freq/sample_rate;
    float alpha = sinf(w0)/(2*q);
    float cosw0 = cosf(w0);
    float sinw0 = sinf(w0);
    return normalise({sinw0/2, 0, -sinw0/2,
                      1+alpha, -2*cosw0, 1-alpha});
}

BiquadCoeffs notch(float freq, float q, float sample_rate) {
    float w0    = 2*kPi*freq/sample_rate;
    float alpha = sinf(w0)/(2*q);
    float cosw0 = cosf(w0);
    return normalise({1, -2*cosw0, 1,
                      1+alpha, -2*cosw0, 1-alpha});
}

BiquadCoeffs low_shelf(float freq, float gain_db, float sample_rate) {
    float A     = powf(10.f, gain_db/40.f);
    float w0    = 2*kPi*freq/sample_rate;
    float cosw0 = cosf(w0);
    float alpha = sinf(w0) * 0.7071068f; // sin(w0)/2 * sqrt(2), S=1
    float sqA   = sqrtf(A);
    return normalise({
        A*((A+1)-(A-1)*cosw0+2*sqA*alpha),
        2*A*((A-1)-(A+1)*cosw0),
        A*((A+1)-(A-1)*cosw0-2*sqA*alpha),
        (A+1)+(A-1)*cosw0+2*sqA*alpha,
        -2*((A-1)+(A+1)*cosw0),
        (A+1)+(A-1)*cosw0-2*sqA*alpha
    });
}

BiquadCoeffs high_shelf(float freq, float gain_db, float sample_rate) {
    float A     = powf(10.f, gain_db/40.f);
    float w0    = 2*kPi*freq/sample_rate;
    float cosw0 = cosf(w0);
    float alpha = sinf(w0) * 0.7071068f;
    float sqA   = sqrtf(A);
    return normalise({
        A*((A+1)+(A-1)*cosw0+2*sqA*alpha),
        -2*A*((A-1)+(A+1)*cosw0),
        A*((A+1)+(A-1)*cosw0-2*sqA*alpha),
        (A+1)-(A-1)*cosw0+2*sqA*alpha,
        2*((A-1)-(A+1)*cosw0),
        (A+1)-(A-1)*cosw0-2*sqA*alpha
    });
}

float BiquadFilter::tick(float in) {
    float out = coeffs.b0*in + s1;
    s1 = coeffs.b1*in - coeffs.a1*out + s2;
    s2 = coeffs.b2*in - coeffs.a2*out;
    return out;
}

void BiquadFilter::set_coeffs(BiquadCoeffs const& c) {
    coeffs = c;
    s1 = s2 = 0;
}

} // namespace synth
