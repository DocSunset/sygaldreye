// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <cmath>
#include <string_view>

// The modular-synth primitive vocabulary. Larger behaviours should be
// subgraphs composed of these, not new C++ nodes.
//
// endpoints v6 throughout. Fields named `in`/`out` would shadow the shape
// templates, so shapes are written `::in`/`::out` — port names stay stable
// (presets and scenes keep working unchanged).

struct LfoNode {
    static consteval std::string_view name() { return "lfo"; }
    struct endpoints {
        normalled_in<float, fp(0.f),    fp(20.f),  fp(0.2f)> freq;
        normalled_in<float, fp(0.f),    fp(100.f), fp(1.f)>  amp;
        normalled_in<float, fp(-100.f), fp(100.f), fp(0.f)>  offset;
        normalled_in<float, fp(0.f),    fp(1.f),   fp(0.f)>  phase;
        ::out<float> out;
    } endpoints;
    void operator()(double t) {
        float ph = float(t) * endpoints.freq.get() + endpoints.phase.get();
        endpoints.out.value = endpoints.offset.get() +
            endpoints.amp.get() * std::sin(ph * 2.f * float(M_PI));
    }
};

struct ScaleNode {
    static consteval std::string_view name() { return "scale"; }
    struct endpoints {
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.f)>  in;
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(-1.f)> in_min;
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(1.f)>  in_max;
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.f)>  out_min;
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(1.f)>  out_max;
        ::out<float> out;
    } endpoints;
    void operator()(double) {
        float den = endpoints.in_max.get() - endpoints.in_min.get();
        float t = (den != 0.f) ? (endpoints.in.get() - endpoints.in_min.get()) / den : 0.f;
        endpoints.out.value = endpoints.out_min.get() +
            t * (endpoints.out_max.get() - endpoints.out_min.get());
    }
};

struct AddNode {
    static consteval std::string_view name() { return "add"; }
    struct endpoints {
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.f)> a, b;
        ::out<float> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.a.get() + endpoints.b.get(); }
};

struct MulNode {
    static consteval std::string_view name() { return "mul"; }
    struct endpoints {
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.f)> a;
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(1.f)> b;
        ::out<float> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.a.get() * endpoints.b.get(); }
};

struct ConstNode {
    static consteval std::string_view name() { return "const"; }
    struct endpoints {
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.f)> value;
        ::out<float> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.value.get(); }
};

struct SubNode {
    static consteval std::string_view name() { return "sub"; }
    struct endpoints {
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.f)> a, b;
        ::out<float> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.a.get() - endpoints.b.get(); }
};

struct DivNode {
    static consteval std::string_view name() { return "div"; }
    struct endpoints {
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.f)> a;
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(1.f)> b;
        ::out<float> out;
    } endpoints;
    void operator()(double) {
        endpoints.out.value = (endpoints.b.get() != 0.f)
            ? endpoints.a.get() / endpoints.b.get() : 0.f;
    }
};

// Linear ramp 0→1 at freq Hz. Phase is internal state: it survives live
// graph edits via migrate_graph, so patching doesn't cause a click.
struct PhasorNode {
    static consteval std::string_view name() { return "phasor"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(100.f), fp(1.f)> freq;
        ::out<float> out;
    } endpoints;
    void operator()(double t) {
        if (prev_t_ >= 0.0) phase_ += float(t - prev_t_) * endpoints.freq.get();
        prev_t_ = t;
        phase_ -= std::floor(phase_);
        endpoints.out.value = phase_;
    }
private:
    float  phase_  = 0.f;
    double prev_t_ = -1.0;
};

// One-pole smoother (de-zippers stepped control signals, e.g. UI sliders).
struct SmoothNode {
    static consteval std::string_view name() { return "smooth"; }
    struct endpoints {
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.f)>  in;
        normalled_in<float, fp(0.001f),  fp(10.f),   fp(0.1f)> time;
        ::out<float> out;
    } endpoints;
    void operator()(double t) {
        float dt = (prev_t_ >= 0.0) ? float(t - prev_t_) : 0.016f;
        prev_t_ = t;
        float k = 1.f - std::exp(-dt / endpoints.time.get());
        state_ += (endpoints.in.get() - state_) * k;
        endpoints.out.value = state_;
    }
private:
    float  state_  = 0.f;
    double prev_t_ = -1.0;
};

#include <Eigen/Core>

// vec3 plumbing: route component signals into/out of vector ports.
struct Split3Node {
    static consteval std::string_view name() { return "split3"; }
    struct endpoints {
        ::in<Eigen::Vector3f> in;
        ::out<float> x, y, z;
    } endpoints;
    void operator()(double) {
        Eigen::Vector3f v = endpoints.in.get();
        endpoints.x.value = v.x();
        endpoints.y.value = v.y();
        endpoints.z.value = v.z();
    }
};

struct Join3Node {
    static consteval std::string_view name() { return "join3"; }
    struct endpoints {
        normalled_in<float, fp(-1000.f), fp(1000.f), fp(0.f)> x, y, z;
        ::out<Eigen::Vector3f> out;
    } endpoints;
    void operator()(double) {
        endpoints.out.value = {endpoints.x.get(), endpoints.y.get(), endpoints.z.get()};
    }
};

// HSV→RGB as vec3 — patch an lfo into hue and everything rainbow-cycles.
struct HsvColorNode {
    static consteval std::string_view name() { return "hsv_color"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)>  h;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.9f)> s;
        normalled_in<float, fp(0.f), fp(1.f), fp(1.f)>  v;
        ::out<Eigen::Vector3f> rgb;
        ::out<float> r, g, b;
    } endpoints;
    void operator()(double) {
        float h = endpoints.h.get() - std::floor(endpoints.h.get());
        float s = endpoints.s.get(), v = endpoints.v.get();
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
        endpoints.rgb.value = {rr + m, gg + m, bb + m};
        endpoints.r.value = rr + m; endpoints.g.value = gg + m; endpoints.b.value = bb + m;
    }
};

// Tick time as a patchable signal, with scale and pause.
struct TimeNode {
    static consteval std::string_view name() { return "time"; }
    struct endpoints {
        normalled_in<float, fp(-10.f), fp(10.f), fp(1.f)> scale;
        ::out<float> t, dt;
    } endpoints;
    void operator()(double t) {
        float dt = (prev_ >= 0.0) ? float(t - prev_) : 0.f;
        prev_ = t;
        scaled_ += dt * endpoints.scale.get();
        endpoints.t.value  = scaled_;
        endpoints.dt.value = dt * endpoints.scale.get();
    }
private:
    double prev_ = -1.0;
    float  scaled_ = 0.f;
};

#include <Eigen/Geometry>

// ── vec3 algebra ────────────────────────────────────────────────────────────
struct Vec3AddNode {
    static consteval std::string_view name() { return "vadd"; }
    struct endpoints {
        ::in<Eigen::Vector3f> a, b;
        ::out<Eigen::Vector3f> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.a.get() + endpoints.b.get(); }
};

struct Vec3ScaleNode {
    static consteval std::string_view name() { return "vscale"; }
    struct endpoints {
        ::in<Eigen::Vector3f> in;
        normalled_in<float, fp(-100.f), fp(100.f), fp(1.f)> k;
        ::out<Eigen::Vector3f> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.in.get() * endpoints.k.get(); }
};

struct Vec3LerpNode {
    static consteval std::string_view name() { return "vlerp"; }
    struct endpoints {
        ::in<Eigen::Vector3f> a, b;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> t;
        ::out<Eigen::Vector3f> out;
    } endpoints;
    void operator()(double) {
        Eigen::Vector3f a = endpoints.a.get();
        endpoints.out.value = a + endpoints.t.get() * (endpoints.b.get() - a);
    }
};

struct Vec3DotNode {
    static consteval std::string_view name() { return "vdot"; }
    struct endpoints {
        ::in<Eigen::Vector3f> a, b;
        ::out<float> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.a.get().dot(endpoints.b.get()); }
};

struct Vec3CrossNode {
    static consteval std::string_view name() { return "vcross"; }
    struct endpoints {
        ::in<Eigen::Vector3f> a, b;
        ::out<Eigen::Vector3f> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.a.get().cross(endpoints.b.get()); }
};

struct Vec3LengthNode {
    static consteval std::string_view name() { return "vlen"; }
    struct endpoints {
        ::in<Eigen::Vector3f> in;
        ::out<float> out;
        ::out<Eigen::Vector3f> normalized;
    } endpoints;
    void operator()(double) {
        Eigen::Vector3f v = endpoints.in.get();
        float n = v.norm();
        endpoints.out.value = n;
        endpoints.normalized.value = (n > 1e-9f) ? Eigen::Vector3f(v / n)
                                                 : Eigen::Vector3f::Zero();
    }
};

// ── quaternions ─────────────────────────────────────────────────────────────
struct QuatEulerNode {
    static consteval std::string_view name() { return "quat_euler"; }
    struct endpoints {
        normalled_in<float, fp(-6.2832f), fp(6.2832f), fp(0.f)> yaw, pitch, roll;
        ::out<Eigen::Quaternionf> out;
    } endpoints;
    void operator()(double) {
        endpoints.out.value =
            Eigen::AngleAxisf(endpoints.yaw.get(),   Eigen::Vector3f::UnitY()) *
            Eigen::AngleAxisf(endpoints.pitch.get(), Eigen::Vector3f::UnitX()) *
            Eigen::AngleAxisf(endpoints.roll.get(),  Eigen::Vector3f::UnitZ());
    }
};

struct QuatMulNode {
    static consteval std::string_view name() { return "quat_mul"; }
    struct endpoints {
        ::in<Eigen::Quaternionf> a, b;   // unwired reads Identity
        ::out<Eigen::Quaternionf> out;
    } endpoints;
    void operator()(double) {
        endpoints.out.value = (endpoints.a.get() * endpoints.b.get()).normalized();
    }
};

struct QuatRotateNode {
    static consteval std::string_view name() { return "quat_rotate"; }
    struct endpoints {
        ::in<Eigen::Quaternionf> q;
        ::in<Eigen::Vector3f>    v;
        ::out<Eigen::Vector3f>   out;
    } endpoints;
    void operator()(double) {
        Eigen::Quaternionf q = endpoints.q.get();
        if (q.norm() < 1e-9f) q.setIdentity();   // wired zero-quat guard
        endpoints.out.value = q.normalized() * endpoints.v.get();
    }
};

struct QuatSlerpNode {
    static consteval std::string_view name() { return "quat_slerp"; }
    struct endpoints {
        ::in<Eigen::Quaternionf> a, b;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> t;
        ::out<Eigen::Quaternionf> out;
    } endpoints;
    void operator()(double) {
        endpoints.out.value = endpoints.a.get().slerp(endpoints.t.get(), endpoints.b.get());
    }
};

// ── transforms ──────────────────────────────────────────────────────────────
struct TrsNode {
    static consteval std::string_view name() { return "trs"; }
    struct endpoints {
        ::in<Eigen::Vector3f>    pos;
        ::in<Eigen::Quaternionf> rot;
        normalled_in<float, fp(0.001f), fp(100.f), fp(1.f)> scale;
        ::out<Eigen::Matrix4f> out;
    } endpoints;
    void operator()(double) {
        Eigen::Quaternionf q = endpoints.rot.get();
        if (q.norm() < 1e-9f) q.setIdentity();   // wired zero-quat guard
        Eigen::Affine3f t = Eigen::Translation3f(endpoints.pos.get()) *
                            q.normalized() *
                            Eigen::Scaling(endpoints.scale.get());
        endpoints.out.value = t.matrix();
    }
};

struct MatMulNode {
    static consteval std::string_view name() { return "mat_mul"; }
    struct endpoints {
        ::in<Eigen::Matrix4f> a, b;   // unwired reads Identity
        ::out<Eigen::Matrix4f> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.a.get() * endpoints.b.get(); }
};
