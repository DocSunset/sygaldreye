// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include "synth_core.hpp"
#include "biquad_filter.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <string_view>
#include <vector>

// The audio unit generators: small block-region nodes the synth presets
// compose (ugen_decomposition.md phase 1). Generators produce the elapsed
// tick's worth of samples (dt-driven, like osc); processors produce
// exactly their input's length. All mono; spatialize/mix handle width.

namespace ugen_detail {
constexpr float kRate = 48000.f;
// Generator boilerplate: elapsed frames since last process, clamped.
struct Gen {
    std::vector<float> buf;
    double prev_t = 0.0;
    int frames(double t) {
        double dt = (prev_t > 0.0) ? t - prev_t : 1.0 / 60.0;
        prev_t = t;
        if (dt <= 0.0 || dt > 0.1) dt = 1.0 / 60.0;
        int n = std::clamp(int(dt * kRate), 0, 4800);
        buf.resize(std::size_t(n));
        return n;
    }
};
} // namespace ugen_detail

// White (color 0) → pink (color 1) noise source.
struct NoiseNode {
    static consteval std::string_view name() { return "noise"; }
    struct inputs {
        slider<"amp",   "", float, fp(0.f), fp(1.f), fp(0.5f)> amp;
        slider<"color", "", float, fp(0.f), fp(1.f), fp(0.f)>  color;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double t) {
        int n = g_.frames(t);
        for (int i = 0; i < n; ++i) {
            float w = synth::white_noise(rng_);
            // Paul Kellet pink approximation, crossfaded by `color`.
            p0_ = 0.99765f * p0_ + w * 0.0990460f;
            p1_ = 0.96300f * p1_ + w * 0.2965164f;
            p2_ = 0.57000f * p2_ + w * 1.0526913f;
            float pink = (p0_ + p1_ + p2_ + w * 0.1848f) * 0.25f;
            g_.buf[std::size_t(i)] =
                inputs.amp.value * ((1.f - inputs.color.value) * w +
                                    inputs.color.value * pink);
        }
        outputs.audio.value = AudioBuffer{g_.buf.data(), n, 1, 48000};
    }
private:
    ugen_detail::Gen g_;
    uint32_t rng_ = 0x1234567u;
    float p0_ = 0.f, p1_ = 0.f, p2_ = 0.f;
};

// Bang-triggered ADSR; emits a per-sample envelope as audio (drive a vca).
struct AdsrNode {
    static consteval std::string_view name() { return "adsr"; }
    struct inputs {
        bang<"trigger"> trigger;
        slider<"gate",    "",  float, fp(0.f),    fp(1.f),  fp(0.f)>    gate;
        slider<"attack",  "s", float, fp(0.001f), fp(2.f),  fp(0.01f)>  attack;
        slider<"decay",   "s", float, fp(0.001f), fp(2.f),  fp(0.1f)>   decay;
        slider<"sustain", "",  float, fp(0.f),    fp(1.f),  fp(0.7f)>   sustain;
        slider<"release", "s", float, fp(0.001f), fp(4.f),  fp(0.2f)>   release;
    } inputs;
    struct outputs {
        port<"audio", AudioBuffer> audio;
        port<"level", float>       level;
    } outputs;
    void operator()(double t) {
        env_.attack_s      = inputs.attack.value;
        env_.decay_s       = inputs.decay.value;
        env_.sustain_level = inputs.sustain.value;
        env_.release_s     = inputs.release.value;
        bool gate = inputs.gate.value > 0.5f;
        if (inputs.trigger.triggered || (gate && !prev_gate_)) env_.gate_on();
        if (!gate && prev_gate_ && !inputs.trigger.triggered) env_.gate_off();
        if (inputs.trigger.triggered && !gate) auto_release_ = true;
        prev_gate_ = gate;
        int n = g_.frames(t);
        for (int i = 0; i < n; ++i) g_.buf[std::size_t(i)] = env_.tick();
        // Bang-fired notes release after the attack+decay completes.
        if (auto_release_ && env_.stage == synth::Adsr::Stage::Sustain) {
            env_.gate_off();
            auto_release_ = false;
        }
        outputs.audio.value = AudioBuffer{g_.buf.data(), n, 1, 48000};
        outputs.level.value = env_.envelope;
    }
private:
    ugen_detail::Gen g_;
    synth::Adsr env_;
    bool prev_gate_ = false, auto_release_ = false;
};

// Multiply audio by a scalar gain and (optionally) an audio-rate envelope.
struct VcaNode {
    static consteval std::string_view name() { return "vca"; }
    struct inputs {
        port<"audio", AudioBuffer> audio;
        port<"env",   AudioBuffer> env;
        slider<"gain", "", float, fp(0.f), fp(4.f), fp(1.f)> gain;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        int n = in.data ? in.frames : 0;
        buf_.resize(std::size_t(n));
        const AudioBuffer& e = inputs.env.value;
        for (int i = 0; i < n; ++i) {
            float g = inputs.gain.value;
            if (e.data && e.frames > 0)
                g *= e.data[std::min(i, e.frames - 1)];
            buf_[std::size_t(i)] = in.data[i] * g;
        }
        outputs.audio.value = AudioBuffer{buf_.data(), n, 1, in.sample_rate};
    }
private:
    std::vector<float> buf_;
};

// Sum up to four mono sources (explicit fan-in; edges don't sum).
struct MixNode {
    static consteval std::string_view name() { return "mix"; }
    struct inputs {
        port<"in1", AudioBuffer> in1;
        port<"in2", AudioBuffer> in2;
        port<"in3", AudioBuffer> in3;
        port<"in4", AudioBuffer> in4;
        slider<"g1", "", float, fp(0.f), fp(2.f), fp(1.f)> g1;
        slider<"g2", "", float, fp(0.f), fp(2.f), fp(1.f)> g2;
        slider<"g3", "", float, fp(0.f), fp(2.f), fp(1.f)> g3;
        slider<"g4", "", float, fp(0.f), fp(2.f), fp(1.f)> g4;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double) {
        const AudioBuffer* ins[4]  = {&inputs.in1.value, &inputs.in2.value,
                                      &inputs.in3.value, &inputs.in4.value};
        float gains[4] = {inputs.g1.value, inputs.g2.value,
                          inputs.g3.value, inputs.g4.value};
        int n = 0, rate = 48000;
        for (auto* b : ins)
            if (b->data) { n = std::max(n, b->frames); rate = b->sample_rate; }
        buf_.assign(std::size_t(n), 0.f);
        for (int s = 0; s < 4; ++s)
            if (ins[s]->data)
                for (int i = 0; i < std::min(n, ins[s]->frames); ++i)
                    buf_[std::size_t(i)] += ins[s]->data[i] * gains[s];
        outputs.audio.value = AudioBuffer{buf_.data(), n, 1, rate};
    }
private:
    std::vector<float> buf_;
};

// Biquad filter node over synth::BiquadFilter. mode: 0 LP, 1 HP, 2 BP.
struct BiquadNode {
    static consteval std::string_view name() { return "biquad"; }
    struct inputs {
        port<"audio", AudioBuffer> audio;
        slider<"freq", "Hz", float, fp(20.f),  fp(20000.f), fp(1000.f)> freq;
        slider<"q",    "",   float, fp(0.3f),  fp(20.f),    fp(0.707f)> q;
        slider<"mode", "",   float, fp(0.f),   fp(2.f),     fp(0.f)>    mode;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        int n = in.data ? in.frames : 0;
        buf_.resize(std::size_t(n));
        int m = int(inputs.mode.value + 0.5f);
        if (inputs.freq.value != freq_ || inputs.q.value != q_ || m != mode_) {
            freq_ = inputs.freq.value; q_ = inputs.q.value; mode_ = m;
            if (m == 1)      f_.set_coeffs(synth::high_pass(freq_, q_, 48000.f));
            else if (m == 2) f_.set_coeffs(synth::band_pass(freq_, q_, 48000.f));
            else             f_.set_coeffs(synth::low_pass(freq_, q_, 48000.f));
        }
        for (int i = 0; i < n; ++i) buf_[std::size_t(i)] = f_.tick(in.data[i]);
        outputs.audio.value = AudioBuffer{buf_.data(), n, 1, in.sample_rate};
    }
private:
    std::vector<float> buf_;
    synth::BiquadFilter f_;
    float freq_ = 0.f, q_ = 0.f;
    int   mode_ = -1;
};

// Sample-accurate delay line with feedback and dry/wet mix.
struct DelayNode {
    static consteval std::string_view name() { return "delay"; }
    struct inputs {
        port<"audio", AudioBuffer> audio;
        slider<"time",     "s", float, fp(0.001f), fp(2.f), fp(0.25f)> time;
        slider<"feedback", "",  float, fp(0.f),    fp(0.98f), fp(0.4f)> feedback;
        slider<"wet",      "",  float, fp(0.f),    fp(1.f),   fp(0.5f)> wet;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    DelayNode() : line_(96000, 0.f) {}
    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        int n = in.data ? in.frames : 0;
        buf_.resize(std::size_t(n));
        int d = std::clamp(int(inputs.time.value * 48000.f), 1, 95999);
        for (int i = 0; i < n; ++i) {
            float echo = line_[std::size_t((write_ - d + 96000) % 96000)];
            float dry  = in.data[i];
            line_[std::size_t(write_)] = dry + echo * inputs.feedback.value;
            write_ = (write_ + 1) % 96000;
            buf_[std::size_t(i)] = dry * (1.f - inputs.wet.value) +
                                   echo * inputs.wet.value;
        }
        outputs.audio.value = AudioBuffer{buf_.data(), n, 1, in.sample_rate};
    }
private:
    std::vector<float> line_, buf_;
    int write_ = 0;
};

// tanh waveshaper (engine growl, saturation).
struct ShaperNode {
    static consteval std::string_view name() { return "shaper"; }
    struct inputs {
        port<"audio", AudioBuffer> audio;
        slider<"drive", "", float, fp(0.1f), fp(20.f), fp(2.f)> drive;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        int n = in.data ? in.frames : 0;
        buf_.resize(std::size_t(n));
        float d = inputs.drive.value, norm = std::tanh(d);
        for (int i = 0; i < n; ++i)
            buf_[std::size_t(i)] = std::tanh(in.data[i] * d) / norm;
        outputs.audio.value = AudioBuffer{buf_.data(), n, 1, in.sample_rate};
    }
private:
    std::vector<float> buf_;
};

// Sample & hold: holds the input, re-sampled `rate` times per second.
struct SampleHoldNode {
    static consteval std::string_view name() { return "sample_hold"; }
    struct inputs {
        port<"audio", AudioBuffer> audio;
        slider<"rate", "Hz", float, fp(0.5f), fp(2000.f), fp(20.f)> rate;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        int n = in.data ? in.frames : 0;
        buf_.resize(std::size_t(n));
        float period = 48000.f / std::max(0.5f, inputs.rate.value);
        for (int i = 0; i < n; ++i) {
            if (++count_ >= period) { held_ = in.data[i]; count_ = 0.f; }
            buf_[std::size_t(i)] = held_;
        }
        outputs.audio.value = AudioBuffer{buf_.data(), n, 1, in.sample_rate};
    }
private:
    std::vector<float> buf_;
    float held_ = 0.f, count_ = 0.f;
};

// Scalar slew limiter: de-zippers latched params (units per second).
struct SlewNode {
    static consteval std::string_view name() { return "slew"; }
    struct inputs {
        slider<"target", "", float, fp(-20000.f), fp(20000.f), fp(0.f)> target;
        slider<"rate", "/s", float, fp(0.01f),    fp(100000.f), fp(10.f)> rate;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double t) {
        double dt = (prev_t_ > 0.0) ? t - prev_t_ : 1.0 / 60.0;
        prev_t_ = t;
        if (dt <= 0.0 || dt > 0.1) dt = 1.0 / 60.0;
        float step = inputs.rate.value * float(dt);
        float d    = inputs.target.value - cur_;
        cur_ += std::clamp(d, -step, step);
        outputs.out.value = cur_;
    }
private:
    double prev_t_ = 0.0;
    float  cur_ = 0.f;
};

// Bang clock: fires `rate` times per second. Inside a synth subgraph it
// drives adsr strikes at block cadence; from the frame region it crosses
// into block through the queue mapping.
struct MetroNode {
    static consteval std::string_view name() { return "metro"; }
    struct inputs {
        slider<"rate", "/s", float, fp(0.f), fp(50.f), fp(1.f)> rate;
    } inputs;
    struct outputs { port<"bang_out", float> bang_out; bang<"fire"> fire; } outputs;
    void operator()(double t) {
        outputs.fire.triggered = false;
        outputs.bang_out.value = 0.f;
        if (inputs.rate.value <= 0.f) return;
        if (next_ == 0.0) next_ = t;
        if (t >= next_) {
            outputs.fire.triggered = true;
            outputs.bang_out.value = 1.f;
            next_ += 1.0 / double(inputs.rate.value);
            if (t >= next_) next_ = t + 1.0 / double(inputs.rate.value);
        }
    }
private:
    double next_ = 0.0;
};

// Stochastic grain generator: the deliberate decomposition floor under
// rain and fire. Sine grains (texture 0) → noise bursts (texture 1) with
// exponential decay; per-grain random pitch spread.
struct GrainCloudNode {
    static consteval std::string_view name() { return "grain_cloud"; }
    struct inputs {
        slider<"rate",    "/s", float, fp(0.f),    fp(2000.f),  fp(100.f)>  rate;
        slider<"freq",    "Hz", float, fp(50.f),   fp(12000.f), fp(3000.f)> freq;
        slider<"spread",  "",   float, fp(0.f),    fp(1.f),     fp(0.5f)>   spread;
        slider<"decay",   "s",  float, fp(0.001f), fp(0.5f),    fp(0.02f)>  decay;
        slider<"texture", "",   float, fp(0.f),    fp(1.f),     fp(0.f)>    texture;
        slider<"amp",     "",   float, fp(0.f),    fp(1.f),     fp(0.5f)>   amp;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double t) {
        int n = g_.frames(t);
        float p_spawn = inputs.rate.value / ugen_detail::kRate;
        float k = 1.f - 1.f / (inputs.decay.value * ugen_detail::kRate);
        for (int i = 0; i < n; ++i) {
            if (synth::white_noise(rng_) * 0.5f + 0.5f < p_spawn) {
                auto& v = voices_[next_voice_++ % kVoices];
                v.env   = 1.f;
                v.phase = 0.f;
                float r = synth::white_noise(rng_);
                v.freq  = inputs.freq.value *
                          (1.f + inputs.spread.value * r * 0.8f);
            }
            float s = 0.f;
            for (auto& v : voices_) {
                if (v.env <= 0.001f) continue;
                float tone  = synth::sine(v.phase);
                float burst = synth::white_noise(rng_);
                s += v.env * ((1.f - inputs.texture.value) * tone +
                              inputs.texture.value * burst);
                v.phase += 6.2831853f * v.freq / ugen_detail::kRate;
                if (v.phase > 6.2831853f) v.phase -= 6.2831853f;
                v.env *= k;
            }
            g_.buf[std::size_t(i)] = inputs.amp.value * s * 0.5f;
        }
        outputs.audio.value = AudioBuffer{g_.buf.data(), n, 1, 48000};
    }
private:
    static constexpr int kVoices = 16;
    struct Voice { float env = 0.f, phase = 0.f, freq = 440.f; };
    ugen_detail::Gen g_;
    Voice voices_[kVoices];
    int next_voice_ = 0;
    uint32_t rng_ = 0xBEEF5EEDu;
};
