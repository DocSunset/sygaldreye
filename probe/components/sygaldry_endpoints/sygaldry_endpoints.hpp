// Copyright 2025 Travis West
#pragma once
#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <string>
#include <string_view>
#include <type_traits>

template <std::size_t N>
struct fixed_string {
    char data[N]{};
    consteval fixed_string(const char (&s)[N]) { std::copy_n(s, N, data); }
    consteval operator std::string_view() const { return {data, N - 1}; }
};
template <std::size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

// Structural float wrapper: allows float literals as NTTPs on NDK Clang 18.
// Float NTTPs are not supported; this bit-casts through int32_t instead.
struct fp {
    std::int32_t bits;
    consteval fp(float f) : bits(std::bit_cast<std::int32_t>(f)) {}
    consteval operator float() const { return std::bit_cast<float>(bits); }
};

// Unwired default for non-arithmetic payloads. Eigen's default ctor is
// UNINITIALIZED — these must say Zero()/Identity() explicitly. (An
// unwired port<vec3> holding heap garbage silenced spatializers
// nondeterministically for two days — see block_swap_poison.md.)
template <typename T>
struct endpoint_default {
    static T value() { return T{}; }
};
template <>
struct endpoint_default<Eigen::Vector2f> {
    static Eigen::Vector2f value() { return Eigen::Vector2f::Zero(); }
};
template <>
struct endpoint_default<Eigen::Vector3f> {
    static Eigen::Vector3f value() { return Eigen::Vector3f::Zero(); }
};
template <>
struct endpoint_default<Eigen::Vector4f> {
    static Eigen::Vector4f value() { return Eigen::Vector4f::Zero(); }
};
template <>
struct endpoint_default<Eigen::Matrix4f> {
    static Eigen::Matrix4f value() { return Eigen::Matrix4f::Identity(); }
};
template <>
struct endpoint_default<Eigen::Quaternionf> {
    static Eigen::Quaternionf value() { return Eigen::Quaternionf::Identity(); }
};

// Audio buffer: borrowed pointer valid only during tick_graph. Samples are
// PLANAR (channel-major): all of channel 0, then channel 1, … — never
// interleaved, except at the device boundary (audio_region sums dacs into
// the interleaved stream). This is the typed audio view of a named-axis
// matrix with axes (Channel × Time); it unifies into Span at the executor
// rung (#59). Planar lets lifted per-channel kernels scan contiguous memory.
struct AudioBuffer {
    const float* data = nullptr;
    int frames = 0;
    int channels = 1;
    int sample_rate = 48000;
};

// Lifting kind (conformability.md): how the executor lifts a node over an
// excess-rank array input. Author-facing constants every node already sees
// (mirror EyeballsLiftKind in the ABI — the descriptor field is a plain int).
// A node opts out of the stateful default with
// `static constexpr int lift_kind() { return lift::resource_holder; }`.
namespace lift {
inline constexpr int stateful = 0;
inline constexpr int stateless = 1;
inline constexpr int resource_holder = 2;
}  // namespace lift

// Axis identity (conformability.md "Audio as named-axis matrix"): what a
// dimension MEANS, so lifting tells a 2-channel buffer from a 2-frame one
// by NAME, not by which count is larger. channels MAP (one kernel instance
// each, per-channel state); time SCANS (one instance, state threaded sample
// to sample). rank ≤ 2 today; the vocabulary grows without widening Span.
enum class Axis : int { Item, Cell, Channel, Time };
inline const char* axis_name(Axis a) {
    switch (a) {
        case Axis::Channel:
            return "channel";
        case Axis::Time:
            return "time";
        case Axis::Cell:
            return "cell";
        case Axis::Item:
            break;
    }
    return "item";
}

// Span: rank-≤2 float view (row-major, rows × cols contiguous). The general
// array payload ratified in conformability.md — lists flow through edges as
// values; lifting builds on this. Axes carry IDENTITY: positions/transforms
// default to Item×Cell (N rows of a vec3/mat4 cell); planar audio is
// Channel×Time. Borrowed pointer, valid during the tick.
struct Span {
    const float* data = nullptr;
    int rows = 0;                // leading-axis length
    int cols = 0;                // trailing-axis length (contiguous)
    Axis row_axis = Axis::Item;  // what the rows MEAN
    Axis col_axis = Axis::Cell;  // what the cols MEAN
};

// ── endpoints v6 (kanban/backlog/endpoints_v6.md) ───────────────────────────
// One `endpoints` struct per node; direction is the shape, names come from
// the field names (boost::pfr core_name). Inputs hold a pointer-to-const
// into the producer's out<T> storage — edges are literal pointers, wired
// once per graph swap. get() is total: unwired reads a default.

// in<T, Def>: connection-only input. No persistence, no widget. The
// natural shape for stream/GPU payloads (audio, texture, mesh) whose
// unconnected state is absence.
template <typename T, fp Def = fp(0.f)>
struct in {
    using value_type = T;
    const T* src = nullptr;
    T get() const {
        if (src) return *src;
        if constexpr (std::is_arithmetic_v<T>)
            return static_cast<T>(static_cast<float>(Def));
        else
            return endpoint_default<T>::value();
    }
};

// whole<T>: a connection-only input that consumes its payload AS A UNIT —
// the lifting opt-out (conformability.md). Same wiring as in<T>, but the
// executor never lifts over it: a reduce/gather/mix/FFT/buffer-as-whole
// port that is fed an N×cell array sees the WHOLE array, not N rows. Audio
// and span ports are whole-by-kind already; whole<T> marks the vector/
// scalar ports that must opt out too.
template <typename T, fp Def = fp(0.f)>
struct whole {
    using value_type = T;
    static constexpr bool whole_array = true;  // schema opt-out marker
    const T* src = nullptr;
    T get() const {
        if (src) return *src;
        if constexpr (std::is_arithmetic_v<T>)
            return static_cast<T>(static_cast<float>(Def));
        else
            return endpoint_default<T>::value();
    }
};

// normalled_in<T, Min, Max, Init>: runtime persisted default + widget
// (patchbay normalling). fallback is the param; an edge overrides it.
// Min/Max/Init are widget metadata, meaningful for arithmetic T only.
template <typename T, fp Min = fp(0.f), fp Max = fp(1.f), fp Init = fp(0.f)>
struct normalled_in {
    using value_type = T;
    const T* src = nullptr;
    T fallback = init();
    T get() const { return src ? *src : fallback; }
    static T init() {
        if constexpr (std::is_arithmetic_v<T>)
            return static_cast<T>(static_cast<float>(Init));
        else
            return endpoint_default<T>::value();
    }
    static consteval float min() { return Min; }
    static consteval float max() { return Max; }
};

// cv_apply customization point: how a modulation source composes with the
// attenuverter params. Affine for arithmetic/vector; slerp-compose for
// quaternions (slope = amount, offset = base).
template <typename T, typename S>
T cv_apply(const T& src, const S& slope, const T& offset) {
    return offset + slope * src;
}
inline Eigen::Quaternionf cv_apply(
    const Eigen::Quaternionf& src, float amount, const Eigen::Quaternionf& base) {
    return base * Eigen::Quaternionf::Identity().slerp(amount, src);
}

// cv_in<T, S>: input with attenuverter params (both persisted). Unwired
// reads offset — the knob IS the unmodulated value.
template <typename T, typename S = float>
struct cv_in {
    using value_type = T;
    const T* src = nullptr;
    T offset = endpoint_default<T>::value();
    S slope = static_cast<S>(1);
    T get() const { return src ? cv_apply(*src, slope, offset) : offset; }
};

// out<T>: owned output storage — what input src pointers point at.
// Default via endpoint_default — Eigen's default ctor is UNINITIALIZED.
template <typename T>
struct out {
    using value_type = T;
    T value = endpoint_default<T>::value();
};

// Events are deliveries, not values: fired for exactly one tick, carried
// across regions by the queue machinery (never dropped, never doubled).
struct event_in {
    bool triggered = false;
};
struct event_out {
    bool triggered = false;
};

// ── v6 shape concepts ────────────────────────────────────────────────────────
template <typename F>
concept V6Input = requires(F f) {
    typename F::value_type;
    { f.src } -> std::convertible_to<const typename F::value_type*>;
    { f.get() } -> std::convertible_to<typename F::value_type>;
};
template <typename F>
concept V6Normalled = V6Input<F> && requires(F f) { f.fallback; };
template <typename F>
concept V6Cv = V6Input<F> && requires(F f) {
    f.offset;
    f.slope;
};
template <typename F>
concept V6Output = requires(F f) {
    typename F::value_type;
    { f.value } -> std::convertible_to<typename F::value_type>;
    requires !V6Input<F>;
};
// A whole<T> input: a V6Input that opts out of lifting (consumes the array
// as a unit). Detected by the static whole_array marker.
template <typename F>
concept V6Whole = V6Input<F> && requires { requires F::whole_array; };
template <typename F>
concept V6EventIn = std::same_as<F, event_in>;
template <typename F>
concept V6EventOut = std::same_as<F, event_out>;

// A v6 node has ONE endpoints struct (inputs/outputs merge; direction is
// the shape). Old two-struct nodes keep working until migrated.
template <typename N>
concept HasEndpoints = requires(N& n) { n.endpoints; };
