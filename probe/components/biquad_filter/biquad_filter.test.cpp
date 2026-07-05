#include "biquad_filter.hpp"
#include <gtest/gtest.h>
#include <cmath>

static constexpr float kPi = 3.14159265358979323846f;

static float rms(float* buf, int n) {
    float sum = 0;
    for (int i = 0; i < n; ++i) sum += buf[i]*buf[i];
    return sqrtf(sum/n);
}

TEST(BiquadFilter, LowPassAttenuatesHighFreq) {
    synth::BiquadFilter f;
    f.set_coeffs(synth::low_pass(1000.f, 0.707f, 48000.f));
    constexpr int N = 1000;
    float in[N], out[N];
    for (int i = 0; i < N; ++i) {
        in[i]  = sinf(2*kPi*10000.f*i/48000.f);
        out[i] = f.tick(in[i]);
    }
    EXPECT_LT(rms(out, N), rms(in, N));
}

TEST(BiquadFilter, BandPassPassesCenterFreq) {
    synth::BiquadFilter f;
    f.set_coeffs(synth::band_pass(1000.f, 5.f, 48000.f));
    constexpr int N = 1000;
    constexpr int SETTLE = 200;
    float in_rms = 0, out_rms = 0;
    for (int i = 0; i < N + SETTLE; ++i) {
        float x = sinf(2*kPi*1000.f*i/48000.f);
        float y = f.tick(x);
        if (i >= SETTLE) {
            in_rms  += x*x;
            out_rms += y*y;
        }
    }
    in_rms  = sqrtf(in_rms/N);
    out_rms = sqrtf(out_rms/N);
    EXPECT_GT(out_rms, in_rms * 0.708f); // within 3 dB
    EXPECT_LT(out_rms, in_rms * 1.414f);
}

TEST(BiquadFilter, HighPassBlocksDC) {
    synth::BiquadFilter f;
    f.set_coeffs(synth::high_pass(1000.f, 0.707f, 48000.f));
    constexpr int N = 1000;
    float last = 0;
    for (int i = 0; i < N; ++i) last = f.tick(1.f);
    EXPECT_NEAR(last, 0.f, 1e-3f);
}
