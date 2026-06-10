// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <cmath>
#include <string_view>

// The modular-synth primitive vocabulary. Larger behaviours should be
// subgraphs composed of these, not new C++ nodes.

struct LfoNode {
    static consteval std::string_view name() { return "lfo"; }
    struct inputs {
        slider<"freq",   "Hz", float, fp(0.f),    fp(20.f),  fp(0.2f)> freq;
        slider<"amp",    "",   float, fp(0.f),    fp(100.f), fp(1.f)>  amp;
        slider<"offset", "",   float, fp(-100.f), fp(100.f), fp(0.f)>  offset;
        slider<"phase",  "",   float, fp(0.f),    fp(1.f),   fp(0.f)>  phase;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double t) {
        float ph = float(t) * inputs.freq.value + inputs.phase.value;
        outputs.out.value = inputs.offset.value +
            inputs.amp.value * std::sin(ph * 2.f * float(M_PI));
    }
};

struct ScaleNode {
    static consteval std::string_view name() { return "scale"; }
    struct inputs {
        slider<"in",      "", float, fp(-1000.f), fp(1000.f), fp(0.f)> in;
        slider<"in_min",  "", float, fp(-1000.f), fp(1000.f), fp(-1.f)> in_min;
        slider<"in_max",  "", float, fp(-1000.f), fp(1000.f), fp(1.f)>  in_max;
        slider<"out_min", "", float, fp(-1000.f), fp(1000.f), fp(0.f)>  out_min;
        slider<"out_max", "", float, fp(-1000.f), fp(1000.f), fp(1.f)>  out_max;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double) {
        float den = inputs.in_max.value - inputs.in_min.value;
        float t = (den != 0.f) ? (inputs.in.value - inputs.in_min.value) / den : 0.f;
        outputs.out.value = inputs.out_min.value +
            t * (inputs.out_max.value - inputs.out_min.value);
    }
};

struct AddNode {
    static consteval std::string_view name() { return "add"; }
    struct inputs {
        slider<"a", "", float, fp(-1000.f), fp(1000.f), fp(0.f)> a;
        slider<"b", "", float, fp(-1000.f), fp(1000.f), fp(0.f)> b;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double) { outputs.out.value = inputs.a.value + inputs.b.value; }
};

struct MulNode {
    static consteval std::string_view name() { return "mul"; }
    struct inputs {
        slider<"a", "", float, fp(-1000.f), fp(1000.f), fp(0.f)> a;
        slider<"b", "", float, fp(-1000.f), fp(1000.f), fp(1.f)> b;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double) { outputs.out.value = inputs.a.value * inputs.b.value; }
};

struct ConstNode {
    static consteval std::string_view name() { return "const"; }
    struct inputs {
        slider<"value", "", float, fp(-1000.f), fp(1000.f), fp(0.f)> value;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double) { outputs.out.value = inputs.value.value; }
};

struct SubNode {
    static consteval std::string_view name() { return "sub"; }
    struct inputs {
        slider<"a", "", float, fp(-1000.f), fp(1000.f), fp(0.f)> a;
        slider<"b", "", float, fp(-1000.f), fp(1000.f), fp(0.f)> b;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double) { outputs.out.value = inputs.a.value - inputs.b.value; }
};

struct DivNode {
    static consteval std::string_view name() { return "div"; }
    struct inputs {
        slider<"a", "", float, fp(-1000.f), fp(1000.f), fp(0.f)> a;
        slider<"b", "", float, fp(-1000.f), fp(1000.f), fp(1.f)> b;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double) {
        outputs.out.value = (inputs.b.value != 0.f) ? inputs.a.value / inputs.b.value : 0.f;
    }
};

// Linear ramp 0→1 at freq Hz. Phase is internal state: it survives live
// graph edits via migrate_graph, so patching doesn't cause a click.
struct PhasorNode {
    static consteval std::string_view name() { return "phasor"; }
    struct inputs {
        slider<"freq", "Hz", float, fp(0.f), fp(100.f), fp(1.f)> freq;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double t) {
        if (prev_t_ >= 0.0) phase_ += float(t - prev_t_) * inputs.freq.value;
        prev_t_ = t;
        phase_ -= std::floor(phase_);
        outputs.out.value = phase_;
    }
private:
    float  phase_  = 0.f;
    double prev_t_ = -1.0;
};

// One-pole smoother (de-zippers stepped control signals, e.g. UI sliders).
struct SmoothNode {
    static consteval std::string_view name() { return "smooth"; }
    struct inputs {
        slider<"in",     "",  float, fp(-1000.f), fp(1000.f), fp(0.f)>   in;
        slider<"time",   "s", float, fp(0.001f),  fp(10.f),   fp(0.1f)>  time;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double t) {
        float dt = (prev_t_ >= 0.0) ? float(t - prev_t_) : 0.016f;
        prev_t_ = t;
        float k = 1.f - std::exp(-dt / inputs.time.value);
        state_ += (inputs.in.value - state_) * k;
        outputs.out.value = state_;
    }
private:
    float  state_  = 0.f;
    double prev_t_ = -1.0;
};
