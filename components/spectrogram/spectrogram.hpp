// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string_view>
#include <vector>

// Rolling STFT spectrogram: audio in, CPU image ring maintained. Each tick
// consumes buffered samples; every hop produces one column (log-magnitude,
// Hann window, radix-2 FFT) written into a ring image (width × kFft/2, R8).
// Frequency runs bottom (DC) to top (Nyquist); time wraps at `columns`.
//
// The former GL texture output is gone — GL lives only in render_region, and
// its viewer (texture_view) is parked. The visual output returns via the
// render_payloads ImageTex path on the offscreen leg
// (kanban/backlog/offscreen_fbo_leg.md); `image()` is the ring to expose.
struct SpectrogramNode {
    static consteval std::string_view name() { return "spectrogram"; }
    static consteval std::string_view source_header() { return "components/spectrogram/spectrogram.hpp"; }

    static constexpr int kFft  = 512;   // bins = 256
    static constexpr int kHop  = 256;

    struct endpoints {
        in<AudioBuffer> audio;
        normalled_in<float, fp(0.1f), fp(100.f),  fp(8.f)>   gain;
        normalled_in<float, fp(64.f), fp(1024.f), fp(256.f)> columns;
        normalled_in<float, fp(0.f),  fp(7.f),    fp(0.f)>   channel;
    } endpoints;

    SpectrogramNode();
    void operator()(double time_s);

    // The ring image: width() columns × kFft/2 rows, one byte per texel
    // (column-major: column c at image()[c * kFft/2]).
    const std::vector<unsigned char>& image() const { return image_; }
    int width() const { return width_; }

private:
    void ensure_size();
    std::vector<float>         accum_, window_;
    std::vector<unsigned char> image_;
    int      width_  = 0;
    int      cursor_ = 0;
};
