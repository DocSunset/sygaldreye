// Copyright 2025 Travis West
#include "fire_synth.hpp"
#include "synth_core.hpp"
#include <cmath>
#include <cstring>

static constexpr float k_tau = 6.28318530f;

static uint32_t xorshift(uint32_t s) {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s;
}

static float rng_float(uint32_t& s) {
    s = xorshift(s);
    return static_cast<float>(s) / static_cast<float>(0xffffffffu);
}

FireSynth::FireSynth(FireParams const& p) {
    params_.store(p);
    roar_bp_.set_coeffs(synth::band_pass(475.0f, 1.0f, p.sample_rate));
    hiss_hp_.set_coeffs(synth::high_pass(2000.0f, 0.7f, p.sample_rate));
    for (auto& c : crackles_) c = {};
}

void FireSynth::set_params(FireParams const& p) {
    params_.store(p);
}

void FireSynth::fill(float* out, int frames) {
    FireParams p = params_.load();
    std::memset(out, 0, static_cast<size_t>(frames) * sizeof(float));

    float freq_inc = k_tau * 0.2f  / p.sample_rate;
    float amp_inc  = k_tau * 0.05f / p.sample_rate;
    float spawn_prob = p.crackle_rate / p.sample_rate;

    static constexpr float k_crackle_decay = 0.005f;

    for (int i = 0; i < frames; ++i) {
        // Roar: band-pass noise with drifting centre and amplitude envelope
        float centre = 475.0f + 100.0f * std::sin(freq_lfo_phase_);
        roar_bp_.set_coeffs(synth::band_pass(centre, 1.0f, p.sample_rate));
        float amp_env = 0.85f + 0.15f * std::sin(amp_lfo_phase_);
        float roar = roar_bp_.tick(synth::white_noise(roar_noise_)) * amp_env * p.intensity;

        freq_lfo_phase_ += freq_inc;
        if (freq_lfo_phase_ >= k_tau) freq_lfo_phase_ -= k_tau;
        amp_lfo_phase_ += amp_inc;
        if (amp_lfo_phase_ >= k_tau) amp_lfo_phase_ -= k_tau;

        // Crackle: Poisson transient pool
        if (rng_float(spawn_rng_) < spawn_prob) {
            for (auto& c : crackles_) {
                if (!c.active) {
                    c.noise_state = xorshift(spawn_rng_);
                    if (c.noise_state == 0) c.noise_state = 1;
                    c.hp.set_coeffs(synth::high_pass(3000.0f, 1.0f, p.sample_rate));
                    c.hp.s1 = c.hp.s2 = 0.0f;
                    c.env   = 1.0f;
                    c.decay = std::exp(-1.0f / (k_crackle_decay * p.sample_rate));
                    c.active = true;
                    break;
                }
            }
        }

        float crackle = 0.0f;
        for (auto& c : crackles_) {
            if (!c.active) continue;
            crackle += c.hp.tick(synth::white_noise(c.noise_state)) * c.env;
            c.env *= c.decay;
            if (c.env < 1e-4f) c.active = false;
        }
        crackle *= p.intensity;

        // Hiss: high-passed noise, low amplitude
        float hiss = hiss_hp_.tick(synth::white_noise(hiss_noise_)) * p.intensity * 0.15f;

        out[i] = roar + crackle + hiss;
    }
}
