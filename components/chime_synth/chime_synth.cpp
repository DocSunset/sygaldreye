// Copyright 2025 Travis West
#include "chime_synth.hpp"
#include <cmath>
#include "synth_core.hpp"

static constexpr float k_two_pi = 6.28318530f;

ChimeSynth::ChimeSynth(ChimeParams const& p)
    : params_{p}, phase_{}, envelope_{}, decay_coeff_{}, poisson_acc_{0.0f},
      pending_strike_gain_{-1.0f}
{
    recompute_coeffs();
}

void ChimeSynth::set_params(ChimeParams const& p)
{
    params_ = p;
    recompute_coeffs();
}

void ChimeSynth::strike(float gain)
{
    pending_strike_gain_.store(gain, std::memory_order_relaxed);
}

void ChimeSynth::apply_strike(float gain)
{
    for (int i = 0; i < params_.mode_count; ++i) {
        envelope_[i] = gain * params_.modes[i].amplitude;
        phase_[i]    = 0.0f;
    }
}

void ChimeSynth::recompute_coeffs()
{
    for (int i = 0; i < params_.mode_count; ++i) {
        float tau = params_.modes[i].decay_s;
        decay_coeff_[i] = (tau > 0.0f)
            ? std::exp(-1.0f / (tau * params_.sample_rate))
            : 0.0f;
    }
}

void ChimeSynth::fill(float* out, int frames)
{
    float pending = pending_strike_gain_.exchange(-1.0f, std::memory_order_relaxed);
    if (pending >= 0.0f) apply_strike(pending);

    float poisson_prob = (params_.strike_rate > 0.0f)
        ? params_.strike_rate / params_.sample_rate
        : 0.0f;

    for (int n = 0; n < frames; ++n) {
        if (poisson_prob > 0.0f) {
            poisson_acc_ += poisson_prob;
            if (poisson_acc_ >= 1.0f) {
                poisson_acc_ -= 1.0f;
                apply_strike(params_.strike_gain);
            }
        }

        float sample = 0.0f;
        for (int i = 0; i < params_.mode_count; ++i) {
            sample      += envelope_[i] * synth::sine(phase_[i]);
            envelope_[i] *= decay_coeff_[i];
            phase_[i]    += k_two_pi * params_.modes[i].freq / params_.sample_rate;
            if (phase_[i] >= k_two_pi) phase_[i] -= k_two_pi;
        }
        out[n] = sample;
    }
}

ChimeParams wind_chime_params()
{
    ChimeParams p;
    p.mode_count  = 5;
    p.strike_rate = 0.0f;
    float freqs[5]  = {440.0f, 1215.0f, 2376.0f, 3929.0f, 5870.0f};
    float amps[5]   = {1.0f, 0.7f, 0.5f, 0.3f, 0.2f};
    float decays[5] = {3.0f, 2.5f, 2.0f, 1.5f, 1.0f};
    for (int i = 0; i < 5; ++i) {
        p.modes[i] = {freqs[i], amps[i], decays[i]};
    }
    return p;
}

ChimeParams bell_params()
{
    ChimeParams p;
    p.mode_count  = 8;
    p.strike_rate = 0.0f;
    float freqs[8]  = {220.0f, 506.0f, 869.0f, 1320.0f, 1892.0f, 2552.0f, 3256.0f, 4004.0f};
    float amps[8]   = {1.0f, 0.8f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f};
    float decays[8] = {8.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.5f, 2.0f, 1.5f};
    for (int i = 0; i < 8; ++i) {
        p.modes[i] = {freqs[i], amps[i], decays[i]};
    }
    return p;
}

ChimeParams robot_click_params()
{
    ChimeParams p;
    p.mode_count  = 3;
    p.strike_rate = 8.0f;
    p.strike_gain = 1.0f;
    float freqs[3]  = {3000.0f, 5000.0f, 8000.0f};
    float amps[3]   = {1.0f, 0.5f, 0.3f};
    float decays[3] = {0.02f, 0.015f, 0.01f};
    for (int i = 0; i < 3; ++i) {
        p.modes[i] = {freqs[i], amps[i], decays[i]};
    }
    return p;
}
