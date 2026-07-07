// Copyright 2025 Travis West
#pragma once
#include <array>
#include <cmath>

// Cosine lookup table with linear interpolation. No state; operator() wraps any angle.
struct CosineLut {
    static constexpr int N = 4096; // power of 2 — enables bitmask wrap in operator()

    static constexpr std::array<float, N> table = []() constexpr noexcept {
        constexpr double pi     = 3.141592653589793;
        constexpr double two_pi = 2.0 * pi;

        // Taylor series accurate to ~10^-11 on [0, π/2]
        auto cos_series = [](double x) constexpr noexcept -> double {
            double x2 = x * x;
            return 1.0 - x2*(0.5 - x2*(1.0/24   - x2*(1.0/720
                 - x2*(1.0/40320 - x2*(1.0/3628800 - x2/479001600.0)))));
        };

        std::array<float, N> t{};
        for (int i = 0; i < N; ++i) {
            double a = two_pi * i / N; // in [0, 2π)
            double c;
            if      (a <= pi * 0.5) c =  cos_series(a);
            else if (a <= pi)       c = -cos_series(pi - a);
            else if (a <= pi * 1.5) c = -cos_series(a - pi);
            else                    c =  cos_series(two_pi - a);
            t[i] = static_cast<float>(c);
        }
        return t;
    }();

    // Linear interpolation between adjacent samples. Handles any finite angle.
    float operator()(float angle) const noexcept {
        constexpr float scale  = N / (2.0f * 3.14159265f);
        float idx = angle * scale;
        // Floor to integer without std::floor: works for both positive and negative
        int i = static_cast<int>(idx);
        if (idx < 0.0f) --i;
        float frac = idx - static_cast<float>(i);
        return table[i & (N - 1)] + frac * (table[(i + 1) & (N - 1)] - table[i & (N - 1)]);
    }
};
