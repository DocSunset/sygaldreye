// Copyright 2024 Travis West
#include "synth_core.hpp"

namespace synth {

static constexpr float k2Pi = 6.28318530f;

float Phasor::tick() {
    float out = phase;
    phase += k2Pi * freq / sample_rate;
    if (phase >= k2Pi) { phase -= k2Pi; }
    return out;
}

void Adsr::gate_on() {
    stage = Stage::Attack;
}

void Adsr::gate_off() {
    if (stage != Stage::Done) { stage = Stage::Release; }
}

float Adsr::tick() {
    switch (stage) {
    case Stage::Attack:
        envelope += 1.0f / (attack_s * sample_rate);
        if (envelope >= 1.0f) {
            envelope = 1.0f;
            stage    = Stage::Decay;
        }
        break;
    case Stage::Decay:
        envelope -= (1.0f - sustain_level) / (decay_s * sample_rate);
        if (envelope <= sustain_level) {
            envelope = sustain_level;
            stage    = Stage::Sustain;
        }
        break;
    case Stage::Sustain:
        envelope = sustain_level;
        break;
    case Stage::Release:
        envelope -= sustain_level / (release_s * sample_rate);
        if (envelope <= 0.0f) {
            envelope = 0.0f;
            stage    = Stage::Done;
        }
        break;
    case Stage::Done:
        envelope = 0.0f;
        break;
    }
    return envelope;
}

float Lfo::tick() {
    return depth * sine(phasor.tick());
}

} // namespace synth
