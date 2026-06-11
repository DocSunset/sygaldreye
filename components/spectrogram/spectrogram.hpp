// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include "gpu_texture.hpp"
#include <string_view>
#include <vector>

// Rolling STFT spectrogram: audio in, texture out. Each tick consumes
// buffered samples; every hop produces one column (log-magnitude, Hann
// window, radix-2 FFT) written into a ring texture. The agent's ears:
// wire any audio edge in, view via texture_view, capture via /screenshot.
// Frequency runs bottom (DC) to top (Nyquist); time wraps at `columns`.
struct SpectrogramNode {
    static consteval std::string_view name() { return "spectrogram"; }
    static consteval std::string_view source_header() { return "components/spectrogram/spectrogram.hpp"; }

    static constexpr int kFft  = 512;   // bins = 256
    static constexpr int kHop  = 256;

    struct inputs {
        port<"audio", AudioBuffer> audio;
        slider<"gain",    "", float, fp(0.1f), fp(100.f), fp(8.f)>   gain;
        slider<"columns", "", float, fp(64.f), fp(1024.f), fp(256.f)> columns;
        slider<"channel", "", float, fp(0.f),  fp(7.f),    fp(0.f)>   channel;
    } inputs;

    struct outputs {
        port<"texture", GpuTexture> texture;
    } outputs;

    SpectrogramNode();
    ~SpectrogramNode();
    void operator()(double time_s);

private:
    void ensure_gl();
    std::vector<float>         accum_, window_;
    std::vector<unsigned char> column_;
    unsigned tex_    = 0;
    int      width_  = 0;
    int      cursor_ = 0;
};
