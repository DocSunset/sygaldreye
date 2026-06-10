#pragma once
#include <cmath>
#include <cstdint>

namespace synth {

inline float sine(float phase)     { return std::sin(phase); }
inline float square(float phase)   { return phase < 3.14159265f ? 1.0f : -1.0f; }
inline float sawtooth(float phase) { return (phase / 3.14159265f) - 1.0f; }
inline float triangle(float phase) {
    float t = phase / 3.14159265f;
    return t < 1.0f ? (2.0f * t - 1.0f) : (3.0f - 2.0f * t);
}

inline float white_noise(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return (static_cast<float>(state) / float(0x80000000u)) - 1.0f;
}

struct Phasor {
    float phase       = 0.0f;
    float freq        = 440.0f;
    float sample_rate = 48000.0f;

    float tick();
};

struct Adsr {
    float attack_s      = 0.01f;
    float decay_s       = 0.1f;
    float sustain_level = 0.7f;
    float release_s     = 0.2f;
    float sample_rate   = 48000.0f;

    enum class Stage { Attack, Decay, Sustain, Release, Done };

    void  gate_on();
    void  gate_off();
    float tick();

    Stage stage    = Stage::Done;
    float envelope = 0.0f;
};

struct Lfo {
    Phasor phasor;
    float  depth = 1.0f;

    float tick();
};

} // namespace synth
