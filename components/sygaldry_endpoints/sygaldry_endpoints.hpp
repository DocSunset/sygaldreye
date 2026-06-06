// Copyright 2025 Travis West
#pragma once
#include <algorithm>
#include <bit>
#include <cstdint>
#include <functional>
#include <string_view>
#include <Eigen/Core>

template<std::size_t N>
struct fixed_string {
    char data[N]{};
    consteval fixed_string(const char (&s)[N]) { std::copy_n(s, N, data); }
    consteval operator std::string_view() const { return {data, N - 1}; }
};
template<std::size_t N> fixed_string(const char (&)[N]) -> fixed_string<N>;

// Structural float wrapper: allows float literals as NTTPs on NDK Clang 18.
// Float NTTPs are not supported; this bit-casts through int32_t instead.
struct fp {
    std::int32_t bits;
    consteval fp(float f) : bits(std::bit_cast<std::int32_t>(f)) {}
    consteval operator float() const { return std::bit_cast<float>(bits); }
};

template<fixed_string Name, fixed_string Desc, typename T, fp Min, fp Max, fp Init>
struct slider {
    static consteval std::string_view name()        { return Name; }
    static consteval std::string_view description() { return Desc; }
    static consteval T                min()         { return static_cast<T>(static_cast<float>(Min)); }
    static consteval T                max()         { return static_cast<T>(static_cast<float>(Max)); }
    static consteval T                init()        { return static_cast<T>(static_cast<float>(Init)); }
    T value = static_cast<T>(static_cast<float>(Init));
};

template<fixed_string Name>
struct toggle {
    static consteval std::string_view name() { return Name; }
    bool value = false;
};

template<fixed_string Name>
struct bang {
    static consteval std::string_view name() { return Name; }
    bool triggered = false;
};

// Generic typed port — carries any value type; no slider metadata.
template<fixed_string Name, typename T>
struct port {
    static consteval std::string_view name() { return Name; }
    using value_type = T;
    T value{};
};

// Audio buffer: borrowed pointer valid only during tick_graph.
struct AudioBuffer {
    const float* data        = nullptr;
    int          frames      = 0;
    int          channels    = 1;
    int          sample_rate = 48000;
};

// Draw call: a renderable callback invoked by the renderer each frame.
using DrawFn = std::function<void(const Eigen::Matrix4f& vp)>;
