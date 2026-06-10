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

#include <Eigen/Core>

// vec3 plumbing: route component signals into/out of vector ports.
struct Split3Node {
    static consteval std::string_view name() { return "split3"; }
    struct inputs  { port<"in", Eigen::Vector3f> in; } inputs;
    struct outputs {
        port<"x", float> x;
        port<"y", float> y;
        port<"z", float> z;
    } outputs;
    void operator()(double) {
        outputs.x.value = inputs.in.value.x();
        outputs.y.value = inputs.in.value.y();
        outputs.z.value = inputs.in.value.z();
    }
};

struct Join3Node {
    static consteval std::string_view name() { return "join3"; }
    struct inputs {
        slider<"x", "", float, fp(-1000.f), fp(1000.f), fp(0.f)> x;
        slider<"y", "", float, fp(-1000.f), fp(1000.f), fp(0.f)> y;
        slider<"z", "", float, fp(-1000.f), fp(1000.f), fp(0.f)> z;
    } inputs;
    struct outputs { port<"out", Eigen::Vector3f> out; } outputs;
    void operator()(double) {
        outputs.out.value = {inputs.x.value, inputs.y.value, inputs.z.value};
    }
};

// HSV→RGB as vec3 — patch an lfo into hue and everything rainbow-cycles.
struct HsvColorNode {
    static consteval std::string_view name() { return "hsv_color"; }
    struct inputs {
        slider<"h", "", float, fp(0.f), fp(1.f), fp(0.f)>  h;
        slider<"s", "", float, fp(0.f), fp(1.f), fp(0.9f)> s;
        slider<"v", "", float, fp(0.f), fp(1.f), fp(1.f)>  v;
    } inputs;
    struct outputs {
        port<"rgb", Eigen::Vector3f> rgb;
        port<"r", float> r;
        port<"g", float> g;
        port<"b", float> b;
    } outputs;
    void operator()(double) {
        float h = inputs.h.value - std::floor(inputs.h.value);
        float s = inputs.s.value, v = inputs.v.value;
        float c = v * s, x = c * (1.f - std::fabs(std::fmod(h * 6.f, 2.f) - 1.f));
        float m = v - c;
        float rr, gg, bb;
        switch (int(h * 6.f) % 6) {
            case 0: rr = c; gg = x; bb = 0; break;
            case 1: rr = x; gg = c; bb = 0; break;
            case 2: rr = 0; gg = c; bb = x; break;
            case 3: rr = 0; gg = x; bb = c; break;
            case 4: rr = x; gg = 0; bb = c; break;
            default: rr = c; gg = 0; bb = x; break;
        }
        outputs.rgb.value = {rr + m, gg + m, bb + m};
        outputs.r.value = rr + m; outputs.g.value = gg + m; outputs.b.value = bb + m;
    }
};

// Tick time as a patchable signal, with scale and pause.
struct TimeNode {
    static consteval std::string_view name() { return "time"; }
    struct inputs {
        slider<"scale", "", float, fp(-10.f), fp(10.f), fp(1.f)> scale;
    } inputs;
    struct outputs {
        port<"t",  float> t;
        port<"dt", float> dt;
    } outputs;
    void operator()(double t) {
        float dt = (prev_ >= 0.0) ? float(t - prev_) : 0.f;
        prev_ = t;
        scaled_ += dt * inputs.scale.value;
        outputs.t.value  = scaled_;
        outputs.dt.value = dt * inputs.scale.value;
    }
private:
    double prev_ = -1.0;
    float  scaled_ = 0.f;
};

#include <Eigen/Geometry>

// ── vec3 algebra ────────────────────────────────────────────────────────────
struct Vec3AddNode {
    static consteval std::string_view name() { return "vadd"; }
    struct inputs  { port<"a", Eigen::Vector3f> a; port<"b", Eigen::Vector3f> b; } inputs;
    struct outputs { port<"out", Eigen::Vector3f> out; } outputs;
    void operator()(double) { outputs.out.value = inputs.a.value + inputs.b.value; }
};

struct Vec3ScaleNode {
    static consteval std::string_view name() { return "vscale"; }
    struct inputs {
        port<"in", Eigen::Vector3f> in;
        slider<"k", "", float, fp(-100.f), fp(100.f), fp(1.f)> k;
    } inputs;
    struct outputs { port<"out", Eigen::Vector3f> out; } outputs;
    void operator()(double) { outputs.out.value = inputs.in.value * inputs.k.value; }
};

struct Vec3LerpNode {
    static consteval std::string_view name() { return "vlerp"; }
    struct inputs {
        port<"a", Eigen::Vector3f> a; port<"b", Eigen::Vector3f> b;
        slider<"t", "", float, fp(0.f), fp(1.f), fp(0.5f)> t;
    } inputs;
    struct outputs { port<"out", Eigen::Vector3f> out; } outputs;
    void operator()(double) {
        outputs.out.value = inputs.a.value +
            inputs.t.value * (inputs.b.value - inputs.a.value);
    }
};

struct Vec3DotNode {
    static consteval std::string_view name() { return "vdot"; }
    struct inputs  { port<"a", Eigen::Vector3f> a; port<"b", Eigen::Vector3f> b; } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double) { outputs.out.value = inputs.a.value.dot(inputs.b.value); }
};

struct Vec3CrossNode {
    static consteval std::string_view name() { return "vcross"; }
    struct inputs  { port<"a", Eigen::Vector3f> a; port<"b", Eigen::Vector3f> b; } inputs;
    struct outputs { port<"out", Eigen::Vector3f> out; } outputs;
    void operator()(double) { outputs.out.value = inputs.a.value.cross(inputs.b.value); }
};

struct Vec3LengthNode {
    static consteval std::string_view name() { return "vlen"; }
    struct inputs  { port<"in", Eigen::Vector3f> in; } inputs;
    struct outputs { port<"out", float> out; port<"normalized", Eigen::Vector3f> normalized; } outputs;
    void operator()(double) {
        float n = inputs.in.value.norm();
        outputs.out.value = n;
        outputs.normalized.value = (n > 1e-9f) ? Eigen::Vector3f(inputs.in.value / n)
                                               : Eigen::Vector3f::Zero();
    }
};

// ── quaternions ─────────────────────────────────────────────────────────────
struct QuatEulerNode {
    static consteval std::string_view name() { return "quat_euler"; }
    struct inputs {
        slider<"yaw",   "", float, fp(-6.2832f), fp(6.2832f), fp(0.f)> yaw;
        slider<"pitch", "", float, fp(-6.2832f), fp(6.2832f), fp(0.f)> pitch;
        slider<"roll",  "", float, fp(-6.2832f), fp(6.2832f), fp(0.f)> roll;
    } inputs;
    struct outputs { port<"out", Eigen::Quaternionf> out; } outputs;
    void operator()(double) {
        outputs.out.value =
            Eigen::AngleAxisf(inputs.yaw.value,   Eigen::Vector3f::UnitY()) *
            Eigen::AngleAxisf(inputs.pitch.value, Eigen::Vector3f::UnitX()) *
            Eigen::AngleAxisf(inputs.roll.value,  Eigen::Vector3f::UnitZ());
    }
};

struct QuatMulNode {
    static consteval std::string_view name() { return "quat_mul"; }
    struct inputs  { port<"a", Eigen::Quaternionf> a; port<"b", Eigen::Quaternionf> b; } inputs;
    struct outputs { port<"out", Eigen::Quaternionf> out; } outputs;
    QuatMulNode() { inputs.a.value.setIdentity(); inputs.b.value.setIdentity(); }
    void operator()(double) {
        outputs.out.value = (inputs.a.value * inputs.b.value).normalized();
    }
};

struct QuatRotateNode {
    static consteval std::string_view name() { return "quat_rotate"; }
    struct inputs  { port<"q", Eigen::Quaternionf> q; port<"v", Eigen::Vector3f> v; } inputs;
    struct outputs { port<"out", Eigen::Vector3f> out; } outputs;
    QuatRotateNode() { inputs.q.value.setIdentity(); }
    void operator()(double) {
        Eigen::Quaternionf q = inputs.q.value;
        if (q.norm() < 1e-9f) q.setIdentity();
        outputs.out.value = q.normalized() * inputs.v.value;
    }
};

struct QuatSlerpNode {
    static consteval std::string_view name() { return "quat_slerp"; }
    struct inputs {
        port<"a", Eigen::Quaternionf> a; port<"b", Eigen::Quaternionf> b;
        slider<"t", "", float, fp(0.f), fp(1.f), fp(0.5f)> t;
    } inputs;
    struct outputs { port<"out", Eigen::Quaternionf> out; } outputs;
    QuatSlerpNode() { inputs.a.value.setIdentity(); inputs.b.value.setIdentity(); }
    void operator()(double) {
        outputs.out.value = inputs.a.value.slerp(inputs.t.value, inputs.b.value);
    }
};

// ── transforms ──────────────────────────────────────────────────────────────
struct TrsNode {
    static consteval std::string_view name() { return "trs"; }
    struct inputs {
        port<"pos",   Eigen::Vector3f>    pos;
        port<"rot",   Eigen::Quaternionf> rot;
        slider<"scale", "", float, fp(0.001f), fp(100.f), fp(1.f)> scale;
    } inputs;
    struct outputs { port<"out", Eigen::Matrix4f> out; } outputs;
    TrsNode() { inputs.rot.value.setIdentity(); }
    void operator()(double) {
        Eigen::Quaternionf q = inputs.rot.value;
        if (q.norm() < 1e-9f) q.setIdentity();
        Eigen::Affine3f t = Eigen::Translation3f(inputs.pos.value) *
                            q.normalized() *
                            Eigen::Scaling(inputs.scale.value);
        outputs.out.value = t.matrix();
    }
};

struct MatMulNode {
    static consteval std::string_view name() { return "mat_mul"; }
    struct inputs  { port<"a", Eigen::Matrix4f> a; port<"b", Eigen::Matrix4f> b; } inputs;
    struct outputs { port<"out", Eigen::Matrix4f> out; } outputs;
    MatMulNode() { inputs.a.value.setIdentity(); inputs.b.value.setIdentity(); }
    void operator()(double) { outputs.out.value = inputs.a.value * inputs.b.value; }
};
