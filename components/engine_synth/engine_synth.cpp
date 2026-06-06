// Copyright 2025 Travis West
#include "engine_synth.hpp"
#include <cmath>

static float fund_hz(EngineParams const& p) {
    return p.rpm / 60.0f * p.cylinders / 2.0f;
}

EngineSynth::EngineSynth(EngineParams const& p) {
    params_.store(p);
    sample_rate_ = p.sample_rate;
    current_rpm_ = p.rpm;
    float sr     = p.sample_rate;
    float f0     = fund_hz(p);
    for (int i = 0; i < k_max_harmonics; ++i) {
        harmonics_[i] = {0.0f, f0 * (i + 1), sr};
    }
    mod_phasor_   = {0.0f, f0, sr};
    noise_filter_.set_coeffs(synth::band_pass(f0 * 2.5f, 2.0f, sr));
}

void EngineSynth::set_params(EngineParams const& p) {
    params_.store(p);
}

void EngineSynth::operator()(double) {
    EngineParams p = params_.load(std::memory_order_relaxed);
    p.rpm       = inputs.rpm.value;
    p.roughness = inputs.roughness.value;
    p.load      = inputs.load.value;
    set_params(p);
}

void EngineSynth::fill(float* out, int frames) {
    EngineParams target = params_.load();
    float smoothing     = 1.0f - std::expf(-1.0f / (0.05f * sample_rate_));

    for (int n = 0; n < frames; ++n) {
        current_rpm_ += (target.rpm - current_rpm_) * smoothing;

        float f0 = current_rpm_ / 60.0f * target.cylinders / 2.0f;
        mod_phasor_.freq = f0;
        for (int i = 0; i < k_max_harmonics; ++i)
            harmonics_[i].freq = f0 * (i + 1);

        float mod_phase = target.roughness * synth::sine(mod_phasor_.tick());

        float sig = 0.0f;
        for (int i = 0; i < k_max_harmonics; ++i) {
            float ph = harmonics_[i].phase + mod_phase;
            float wrap = std::fmod(ph, 2.0f * 3.14159265f);
            if (wrap < 0.0f) wrap += 2.0f * 3.14159265f;
            sig += synth::sine(wrap) * (1.0f / (i + 1)) * target.load;
            harmonics_[i].tick();
        }

        float noise = synth::white_noise(noise_state_);
        sig += noise_filter_.tick(noise) * 0.1f;

        out[n] = sig / k_max_harmonics;
    }

}
