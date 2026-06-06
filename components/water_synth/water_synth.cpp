// Copyright 2025 Travis West
#include "water_synth.hpp"
#include <cstring>

WaterSynth::WaterSynth(WaterSynthParams const& p) {
    params_.store(p, std::memory_order_relaxed);
    lfo_a_.phasor.freq        = 0.07f;
    lfo_a_.phasor.sample_rate = p.sample_rate;
    lfo_a_.depth              = 1.0f;
    lfo_b_.phasor.freq        = 0.13f;
    lfo_b_.phasor.sample_rate = p.sample_rate;
    lfo_b_.depth              = 1.0f;
    wave_phasor_.freq         = p.wave_rate;
    wave_phasor_.sample_rate  = p.sample_rate;
    float centre = 400.0f + p.brightness * 800.0f;
    bp_.set_coeffs(synth::band_pass(centre, 2.0f, p.sample_rate));
    for (auto& s : splashes_) {
        s.lp.set_coeffs(synth::low_pass(3000.0f, 0.707f, p.sample_rate));
        s.decay = std::exp(-1.0f / (0.08f * p.sample_rate));
    }
}

void WaterSynth::set_params(WaterSynthParams const& p) {
    params_.store(p, std::memory_order_relaxed);
}

void WaterSynth::operator()(double) {
    WaterSynthParams p = params_.load(std::memory_order_relaxed);
    p.flow_speed  = inputs.flow_speed.value;
    p.wave_rate   = inputs.wave_rate.value;
    p.wave_height = inputs.wave_height.value;
    p.brightness  = inputs.brightness.value;
    set_params(p);
}

void WaterSynth::fill(float* out, int frames) {
    std::memset(out, 0, static_cast<size_t>(frames) * sizeof(float));
    WaterSynthParams p = params_.load(std::memory_order_relaxed);

    lfo_a_.phasor.sample_rate = p.sample_rate;
    lfo_b_.phasor.sample_rate = p.sample_rate;
    wave_phasor_.sample_rate  = p.sample_rate;
    wave_phasor_.freq         = p.wave_rate;

    float splash_decay = std::exp(-1.0f / (0.08f * p.sample_rate));
    float centre_base  = 400.0f + p.brightness * 800.0f;
    float lfo_depth    = p.brightness * 200.0f;

    for (int i = 0; i < frames; ++i) {
        float la = lfo_a_.tick();
        float lb = lfo_b_.tick();
        float centre = centre_base + lfo_depth * (la + lb);
        if (centre < 20.0f) centre = 20.0f;
        bp_.set_coeffs(synth::band_pass(centre, 2.0f, p.sample_rate));

        float noise    = synth::white_noise(noise_state_);
        float flow     = bp_.tick(noise) * p.flow_speed;

        float wave_phase = (p.wave_rate > 0.0f) ? wave_phasor_.tick() : 0.0f;
        wave_env_ = (1.0f - p.wave_height)
                  + p.wave_height * 0.5f * (1.0f + synth::sine(wave_phase));

        bool above = wave_env_ > 0.9f;
        if (above && !wave_above_) {
            for (auto& s : splashes_) {
                if (!s.active) {
                    s.env         = 1.0f;
                    s.noise_state = noise_state_ ^ 0xabcd1234u;
                    s.lp.s1 = s.lp.s2 = 0.0f;
                    s.lp.set_coeffs(synth::low_pass(3000.0f, 0.707f, p.sample_rate));
                    s.decay = splash_decay;
                    s.active = true;
                    break;
                }
            }
        }
        wave_above_ = above;

        float splash = 0.0f;
        for (auto& s : splashes_) {
            if (!s.active) continue;
            float sn = synth::white_noise(s.noise_state);
            splash += s.lp.tick(sn) * s.env;
            s.env *= s.decay;
            if (s.env < 1e-5f) s.active = false;
        }

        out[i] = flow * wave_env_ + splash * p.wave_height;
    }
}
