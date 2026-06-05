// Copyright 2025 Travis West
#include "creature_synth.hpp"
#include "synth_core.hpp"
#include <cmath>

namespace {
constexpr float kTwoPi = 6.28318530f;
}

CreatureSynth::CreatureSynth(CreatureParams const& p) {
    set_params(p);
    if (p.kind == CreatureKind::Bird) {
        adsr_.attack_s      = 0.05f;
        adsr_.decay_s       = 0.001f;
        adsr_.sustain_level = 1.0f;
        adsr_.release_s     = 0.2f;
        adsr_.gate_on();
    }
}

void CreatureSynth::set_params(CreatureParams const& p) {
    params_ = p;
    carrier_.sample_rate   = p.sample_rate;
    carrier_.freq          = p.carrier_freq;
    modulator_.sample_rate = p.sample_rate;
    modulator_.freq        = p.carrier_freq;
    adsr_.sample_rate      = p.sample_rate;
    group_phasor_.sample_rate = p.sample_rate;
    group_phasor_.freq        = 0.8f;
    wing_.sample_rate      = p.sample_rate;
    wing_.freq             = 300.0f;
}

static void tick_phasor(synth::Phasor& p) {
    p.phase += kTwoPi * p.freq / p.sample_rate;
    if (p.phase >= kTwoPi) p.phase -= kTwoPi;
}

void CreatureSynth::fill(float* out, int frames) {
    const CreatureParams& p = params_;

    for (int i = 0; i < frames; ++i) {
        float sample = 0.0f;

        if (p.kind == CreatureKind::Cricket) {
            float group_env = synth::sine(group_phasor_.phase) * 0.5f + 0.5f;
            tick_phasor(group_phasor_);

            if (group_env > 0.5f) {
                chirp_accum_ += p.chirp_rate / p.sample_rate;
                if (chirp_accum_ >= 1.0f) {
                    chirp_accum_ -= 1.0f;
                    adsr_.attack_s      = 0.005f;
                    adsr_.decay_s       = 0.020f;
                    adsr_.sustain_level = 0.0f;
                    adsr_.release_s     = 0.001f;
                    adsr_.gate_on();
                    adsr_.gate_off();
                }
            }

            float mod_phase = modulator_.phase;
            tick_phasor(modulator_);
            float car_phase = carrier_.phase + p.fm_index * synth::sine(mod_phase);
            tick_phasor(carrier_);
            sample = adsr_.tick() * synth::sine(car_phase);

        } else if (p.kind == CreatureKind::Bird) {
            float step_dur = p.phrase_duration_s / 8.0f;
            contour_phase_ += 1.0f / (step_dur * p.sample_rate);
            if (contour_phase_ >= 1.0f) {
                contour_phase_ -= 1.0f;
                contour_idx_ = (contour_idx_ + 1) % 8;
                if (contour_idx_ == 0) {
                    adsr_.attack_s      = 0.05f;
                    adsr_.decay_s       = 0.001f;
                    adsr_.sustain_level = 1.0f;
                    adsr_.release_s     = 0.2f;
                    adsr_.gate_on();
                }
                if (contour_idx_ == 7) adsr_.gate_off();
            }
            int next_idx = (contour_idx_ + 1) % 8;
            float freq = p.pitch_contour[contour_idx_] * (1.0f - contour_phase_)
                       + p.pitch_contour[next_idx]    * contour_phase_;
            carrier_.freq = freq;
            tick_phasor(carrier_);
            sample = adsr_.tick() * (synth::sine(carrier_.phase)
                                   + 0.5f * synth::sine(2.0f * carrier_.phase));

        } else { // Insect
            float mod_phase = modulator_.phase;
            tick_phasor(modulator_);
            float car_phase = carrier_.phase + p.fm_index * synth::sine(mod_phase);
            tick_phasor(carrier_);
            float wing_env = 0.5f * (1.0f + synth::sine(wing_.phase));
            tick_phasor(wing_);
            sample = wing_env * synth::sine(car_phase);
        }

        out[i] = sample;
    }
}
