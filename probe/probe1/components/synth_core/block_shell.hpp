// Copyright 2026 Travis West
#pragma once
#include <algorithm>
#include <vector>

#include "sygaldry_endpoints.hpp"  // AudioBuffer

// Generator block shell (kernel_extraction.md): the symmetric partner to
// ugen_detail::Lift. A source authors only a per-sample body; this shell
// owns the dt→frame-count clamping (NO absolute-time state ever reaches a
// kernel — the metro epoch bug class is structurally impossible) and the
// mono AudioBuffer. "Block processing is the executor's lifting loop"
// (conformability.md) — for generators, this loop is it.

namespace synth {

inline constexpr float kSampleRate = 48000.f;

struct Gen {
    std::vector<float> buf;
    double prev_t = 0.0;

    // Elapsed frames since the last call, clamped against pauses/seeks.
    int frames(double t) {
        double dt = (prev_t > 0.0) ? t - prev_t : 1.0 / 60.0;
        prev_t = t;
        if (dt <= 0.0 || dt > 0.1) dt = 1.0 / 60.0;
        int n = std::clamp(int(dt * kSampleRate), 0, 4800);
        buf.resize(std::size_t(n));
        return n;
    }

    // Fill n mono frames by calling body() per sample.
    template <typename Body>
    AudioBuffer generate(double t, Body&& body) {
        int n = frames(t);
        for (int i = 0; i < n; ++i) buf[std::size_t(i)] = body();
        return AudioBuffer{buf.data(), n, 1, int(kSampleRate)};
    }
};

}  // namespace synth
