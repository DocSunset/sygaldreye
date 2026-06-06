#include "atmos_synth.hpp"
#include "synth_core.hpp"
#include <cmath>

static constexpr float k_tau = 6.28318530f;

AtmosSynth::AtmosSynth(AtmosParams const& p) {
    params_.store(p);
    auto c = synth::band_pass(p.base_freq, 2.0f, p.sample_rate);
    wind_filter_.set_coeffs(c);
    auto cw = synth::band_pass(p.base_freq * p.brightness * 4.0f, 8.0f, p.sample_rate);
    whistle_filter_.set_coeffs(cw);
}

void AtmosSynth::set_params(AtmosParams const& p) {
    params_.store(p);
}

void AtmosSynth::operator()(double) {
    AtmosParams p = params_.load(std::memory_order_relaxed);
    p.wind_speed = inputs.wind_speed.value;
    p.base_freq  = inputs.base_freq.value;
    p.brightness = inputs.brightness.value;
    set_params(p);
}

void AtmosSynth::fill(float* out, int frames) {
    AtmosParams p = params_.load();

    float freq_lfo_inc = k_tau * 0.2f / p.sample_rate;
    float amp_lfo_inc  = k_tau * 0.05f / p.sample_rate;

    for (int i = 0; i < frames; ++i) {
        float freq_mod = 1.0f + 0.5f * std::sin(freq_lfo_phase_);
        float centre   = p.base_freq * freq_mod;

        wind_filter_.set_coeffs(synth::band_pass(centre, 2.0f, p.sample_rate));
        float whistle_freq = p.base_freq * p.brightness * 4.0f * freq_mod;
        whistle_filter_.set_coeffs(synth::band_pass(whistle_freq, 8.0f, p.sample_rate));

        float n       = synth::white_noise(noise_state_);
        float wind    = wind_filter_.tick(n);
        float whistle = whistle_filter_.tick(n) * p.brightness;

        float amp_env = 0.5f + 0.5f * std::sin(amp_lfo_phase_);
        out[i] = (wind + whistle) * amp_env * p.wind_speed;

        freq_lfo_phase_ += freq_lfo_inc;
        if (freq_lfo_phase_ >= k_tau) freq_lfo_phase_ -= k_tau;
        amp_lfo_phase_ += amp_lfo_inc;
        if (amp_lfo_phase_ >= k_tau) amp_lfo_phase_ -= k_tau;
    }
}
