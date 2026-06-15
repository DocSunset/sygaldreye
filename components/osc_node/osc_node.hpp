// Copyright 2026 Travis West
#pragma once
#include <string_view>

#include "block_shell.hpp"
#include "sygaldry_endpoints.hpp"
#include "synth_core.hpp"

// Frame-rate oscillator: generates the elapsed tick's worth of samples
// each process (phase-continuous) and emits them as an audio edge. Test
// source for spectrogram/dac until block-rate synths land; stays useful
// afterwards as the frame-region reference tone. The DSP is the synth::
// Phasor kernel + shape functions; the block shell is synth::Gen
// (kernel_extraction.md) — no hand-written loop, no inline phase state.
struct OscNode {
    static consteval std::string_view name() { return "osc"; }
    static consteval std::string_view source_header() { return "components/osc_node/osc_node.hpp"; }

    struct endpoints {
        normalled_in<float, fp(20.f), fp(20000.f), fp(440.f)> freq;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> amp;
        normalled_in<float, fp(0.f), fp(3.f), fp(0.f)> wave;  // sine saw square tri
        out<AudioBuffer> audio;
    } endpoints;

    void operator()(double time_s) {
        phasor_.freq = endpoints.freq.get();
        phasor_.sample_rate = synth::kSampleRate;
        float amp = endpoints.amp.get();
        int wave = int(endpoints.wave.get() + 0.5f);
        endpoints.audio.value = g_.generate(time_s, [&] {
            float ph = phasor_.tick();
            float s = wave == 1   ? synth::sawtooth(ph)
                      : wave == 2 ? synth::square(ph)
                      : wave == 3 ? synth::triangle(ph)
                                  : synth::sine(ph);
            return amp * s;
        });
    }

private:
    synth::Gen g_;
    synth::Phasor phasor_;
};
