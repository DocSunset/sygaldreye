// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include "synth_core.hpp"
#include <cmath>
#include <string_view>
#include <vector>

// Frame-rate sine oscillator: generates the elapsed tick's worth of
// samples each process (phase-continuous) and emits them as an audio
// edge. Test source for spectrogram/dac until block-rate synths land;
// stays useful afterwards as the frame-region reference tone.
struct OscNode {
    static consteval std::string_view name() { return "osc"; }
    static consteval std::string_view source_header() { return "components/osc_node/osc_node.hpp"; }

    struct endpoints {
        normalled_in<float, fp(20.f), fp(20000.f), fp(440.f)> freq;
        normalled_in<float, fp(0.f),  fp(1.f),     fp(0.5f)>  amp;
        normalled_in<float, fp(0.f),  fp(3.f),     fp(0.f)>   wave;  // sine saw square tri
        out<AudioBuffer> audio;
    } endpoints;

    void operator()(double time_s) {
        double dt = (prev_t_ > 0.0) ? time_s - prev_t_ : 1.0 / 60.0;
        prev_t_ = time_s;
        int frames = std::clamp(int(dt * 48000.0), 0, 4800);
        buf_.resize(std::size_t(frames));
        float w = 6.2831853f * endpoints.freq.get() / 48000.f;
        float amp = endpoints.amp.get();
        int wave = int(endpoints.wave.get() + 0.5f);
        for (int i = 0; i < frames; ++i) {
            float s = wave == 1 ? synth::sawtooth(phase_)
                    : wave == 2 ? synth::square(phase_)
                    : wave == 3 ? synth::triangle(phase_)
                                : std::sin(phase_);
            buf_[std::size_t(i)] = amp * s;
            phase_ += w;
        }
        phase_ = std::fmod(phase_, 6.2831853f);
        endpoints.audio.value = AudioBuffer{buf_.data(), frames, 1, 48000};
    }

private:
    std::vector<float> buf_;
    double prev_t_ = 0.0;
    float  phase_  = 0.f;
};
