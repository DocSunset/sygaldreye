// Copyright 2025 Travis West
#include "rain_synth.hpp"
#include <cstring>

static uint32_t xorshift(uint32_t s) {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s;
}

static float rng_float(uint32_t& s) {
    s = xorshift(s);
    return static_cast<float>(s) / static_cast<float>(0xffffffffu);
}

RainSynth::RainSynth(RainParams const& p) : spawn_rng_(0xdeadbeef) {
    for (auto& d : drops_) d = {};
    set_params(p);
}

void RainSynth::set_params(RainParams const& p) {
    params_ = p;
}

void RainSynth::operator()(double) {
    RainParams p = params_;
    p.rate      = inputs.rate.value;
    p.drop_freq = inputs.drop_freq.value;
    p.decay_s   = inputs.decay_s.value;
    set_params(p);
}

void RainSynth::fill(float* out, int frames) {
    std::memset(out, 0, static_cast<size_t>(frames) * sizeof(float));
    float spawn_prob = params_.rate / params_.sample_rate;
    for (int i = 0; i < frames; ++i) {
        if (rng_float(spawn_rng_) < spawn_prob) {
            for (auto& d : drops_) {
                if (!d.active) {
                    d.noise_state = xorshift(spawn_rng_);
                    if (d.noise_state == 0) d.noise_state = 1;
                    d.env.attack_s      = 0.0005f;
                    d.env.decay_s       = params_.decay_s;
                    d.env.sustain_level = 0.0f;
                    d.env.release_s     = 0.0f;
                    d.env.sample_rate   = params_.sample_rate;
                    d.lp.set_coeffs(synth::low_pass(params_.drop_freq, 0.707f, params_.sample_rate));
                    d.lp.s1 = d.lp.s2 = 0.0f;
                    d.env.gate_on();
                    d.env.gate_off();
                    d.active = true;
                    break;
                }
            }
        }
        float sample = 0.0f;
        for (auto& d : drops_) {
            if (!d.active) continue;
            float noise    = synth::white_noise(d.noise_state);
            float filtered = d.lp.tick(noise);
            float env      = d.env.tick();
            sample += filtered * env;
            if (d.env.stage == synth::Adsr::Stage::Done)
                d.active = false;
        }
        out[i] = sample;
    }
}
