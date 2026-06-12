// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include "synth_core.hpp"
#include "kernels.hpp"
#include "biquad_filter.hpp"
#include <algorithm>
#include <cmath>
#include <string_view>
#include <vector>

// The audio unit generators (ugen_decomposition.md phase 1+).
//
// MULTICHANNEL, Pd-style: an audio edge carries any number of interleaved
// channels, decided by what's wired (AudioBuffer.channels). Processors
// adapt transparently — per-channel state grows to match the input; a
// mono side-input broadcasts; mismatched counts wrap (c % channels).
// Generators are mono; mc_pack concatenates channel lists and mc_unpack
// peels them when explicit decomposition is wanted.
//
// Generators produce the elapsed tick's worth of samples (dt-driven);
// processors produce exactly their input's frame count.

namespace ugen_detail {
constexpr float kRate = 48000.f;

struct Gen {  // generator boilerplate: elapsed frames, clamped
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

inline int chans(const AudioBuffer& b) { return std::max(1, b.channels); }
// Sample of channel c, wrapping into the buffer's own channel count
// (mono broadcasts, mismatches wrap).
inline float at(const AudioBuffer& b, int frame, int c) {
    return b.data[std::size_t(frame) * std::size_t(chans(b)) +
                  std::size_t(c % chans(b))];
}
} // namespace ugen_detail

// White (color 0) → pink (color 1) noise source. Mono.
struct NoiseNode {
    static consteval std::string_view name() { return "noise"; }
    struct inputs {
        slider<"amp",   "", float, fp(0.f), fp(1.f), fp(0.5f)> amp;
        slider<"color", "", float, fp(0.f), fp(1.f), fp(0.f)>  color;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double t) {
        int n = g_.frames(t);
        for (int i = 0; i < n; ++i)
            g_.buf[std::size_t(i)] = inputs.amp.value * k_.tick(inputs.color.value);
        outputs.audio.value = AudioBuffer{g_.buf.data(), n, 1, 48000};
    }
private:
    ugen_detail::Gen g_;
    synth::PinkNoise k_;
};

// Percussive one-shot envelope: bang (or gate rise) → attack/decay,
// auto-release. The pluck/strike envelope; for held notes use `adsr`.
struct PercNode {
    static consteval std::string_view name() { return "perc"; }
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

// CV-style ADSR: the gate IS an audio signal (sample-accurate, threshold
// 0.5); sustain holds while the gate holds. Per-channel envelopes — a
// multichannel gate yields a multichannel envelope.
struct AdsrNode {
    static consteval std::string_view name() { return "adsr"; }
    struct inputs {
        port<"gate", AudioBuffer> gate;
        slider<"attack",  "s", float, fp(0.001f), fp(2.f), fp(0.01f)> attack;
        slider<"decay",   "s", float, fp(0.001f), fp(2.f), fp(0.1f)>  decay;
        slider<"sustain", "",  float, fp(0.f),    fp(1.f), fp(0.7f)>  sustain;
        slider<"release", "s", float, fp(0.001f), fp(4.f), fp(0.2f)>  release;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double) {
        const AudioBuffer& gate = inputs.gate.value;
        int n  = gate.data ? gate.frames : 0;
        int ch = ugen_detail::chans(gate);
        if (int(envs_.size()) != ch) {
            envs_.assign(std::size_t(ch), synth::Adsr{});
            prev_.assign(std::size_t(ch), 0);
        }
        for (auto& e : envs_) {
            e.attack_s      = inputs.attack.value;
            e.decay_s       = inputs.decay.value;
            e.sustain_level = inputs.sustain.value;
            e.release_s     = inputs.release.value;
        }
        buf_.resize(std::size_t(n) * std::size_t(ch));
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < ch; ++c) {
                bool g = ugen_detail::at(gate, i, c) > 0.5f;
                if (g && !prev_[std::size_t(c)]) envs_[std::size_t(c)].gate_on();
                if (!g && prev_[std::size_t(c)]) envs_[std::size_t(c)].gate_off();
                prev_[std::size_t(c)] = g;
                buf_[std::size_t(i) * std::size_t(ch) + std::size_t(c)] =
                    envs_[std::size_t(c)].tick();
            }
        outputs.audio.value = AudioBuffer{buf_.data(), n, ch, gate.sample_rate};
    }
private:
    std::vector<float>       buf_;
    std::vector<synth::Adsr> envs_;
    std::vector<char>        prev_;
};

// Multiply audio by a scalar gain and (optionally) an audio-rate envelope
// (mono env broadcasts; otherwise channel-wise).
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
        int n  = in.data ? in.frames : 0;
        int ch = ugen_detail::chans(in);
        buf_.resize(std::size_t(n) * std::size_t(ch));
        const AudioBuffer& e = inputs.env.value;
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < ch; ++c) {
                float g = inputs.gain.value;
                if (e.data && e.frames > 0)
                    g *= ugen_detail::at(e, std::min(i, e.frames - 1), c);
                buf_[std::size_t(i) * std::size_t(ch) + std::size_t(c)] =
                    ugen_detail::at(in, i, c) * g;
            }
        outputs.audio.value = AudioBuffer{buf_.data(), n, ch, in.sample_rate};
    }
private:
    std::vector<float> buf_;
};

// Sum up to four sources channel-wise (mono inputs broadcast; output
// carries the widest input's channel count).
struct MixNode {
    static consteval std::string_view name() { return "mix"; }
    // endpoints v6: unconnected in<AudioBuffer> is structurally silent —
    // a deleted producer can never leave a stale view droning here.
    struct endpoints {
        in<AudioBuffer> in1, in2, in3, in4;
        normalled_in<float, fp(0.f), fp(2.f), fp(1.f)> g1, g2, g3, g4;
        out<AudioBuffer> audio;
    } endpoints;
    void operator()(double) {
        const AudioBuffer bufs[4] = {endpoints.in1.get(), endpoints.in2.get(),
                                     endpoints.in3.get(), endpoints.in4.get()};
        float gains[4] = {endpoints.g1.get(), endpoints.g2.get(),
                          endpoints.g3.get(), endpoints.g4.get()};
        int n = 0, ch = 1, rate = 48000;
        for (const auto& b : bufs)
            if (b.data && b.frames > 0) {
                n    = std::max(n, b.frames);
                ch   = std::max(ch, ugen_detail::chans(b));
                rate = b.sample_rate;
            }
        buf_.assign(std::size_t(n) * std::size_t(ch), 0.f);
        for (int s = 0; s < 4; ++s) {
            if (!bufs[s].data) continue;
            for (int i = 0; i < std::min(n, bufs[s].frames); ++i)
                for (int c = 0; c < ch; ++c)
                    buf_[std::size_t(i) * std::size_t(ch) + std::size_t(c)] +=
                        ugen_detail::at(bufs[s], i, c) * gains[s];
        }
        endpoints.audio.value = AudioBuffer{buf_.data(), n, ch, rate};
    }
private:
    std::vector<float> buf_;
};

// Biquad filter, per-channel state. mode: 0 LP, 1 HP, 2 BP.
struct BiquadNode {
    static consteval std::string_view name() { return "biquad"; }
    struct inputs {
        port<"audio", AudioBuffer> audio;
        slider<"freq", "Hz", float, fp(20.f), fp(20000.f), fp(1000.f)> freq;
        slider<"q",    "",   float, fp(0.3f), fp(20.f),    fp(0.707f)> q;
        slider<"mode", "",   float, fp(0.f),  fp(2.f),     fp(0.f)>    mode;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        int n  = in.data ? in.frames : 0;
        int ch = ugen_detail::chans(in);
        if (int(filters_.size()) != ch) {
            filters_.assign(std::size_t(ch), synth::BiquadFilter{});
            coeffs_stale_ = true;
        }
        int m = int(inputs.mode.value + 0.5f);
        if (inputs.freq.value != freq_ || inputs.q.value != q_ || m != mode_ ||
            coeffs_stale_) {
            freq_ = inputs.freq.value; q_ = inputs.q.value; mode_ = m;
            coeffs_stale_ = false;
            synth::BiquadCoeffs co =
                m == 1 ? synth::high_pass(freq_, q_, 48000.f)
              : m == 2 ? synth::band_pass(freq_, q_, 48000.f)
                       : synth::low_pass(freq_, q_, 48000.f);
            for (auto& f : filters_) f.set_coeffs(co);
        }
        buf_.resize(std::size_t(n) * std::size_t(ch));
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < ch; ++c)
                buf_[std::size_t(i) * std::size_t(ch) + std::size_t(c)] =
                    filters_[std::size_t(c)].tick(ugen_detail::at(in, i, c));
        outputs.audio.value = AudioBuffer{buf_.data(), n, ch, in.sample_rate};
    }
private:
    std::vector<float> buf_;
    std::vector<synth::BiquadFilter> filters_;
    float freq_ = 0.f, q_ = 0.f;
    int   mode_ = -1;
    bool  coeffs_stale_ = true;  // also set when the channel count grows
};

// Sample-accurate delay with feedback, per-channel lines.
struct DelayNode {
    static consteval std::string_view name() { return "delay"; }
    struct inputs {
        port<"audio", AudioBuffer> audio;
        slider<"time",     "s", float, fp(0.001f), fp(2.f),   fp(0.25f)> time;
        slider<"feedback", "",  float, fp(0.f),    fp(0.98f), fp(0.4f)>  feedback;
        slider<"wet",      "",  float, fp(0.f),    fp(1.f),   fp(0.5f)>  wet;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    static constexpr int kLen = 96000;
    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        int n  = in.data ? in.frames : 0;
        int ch = ugen_detail::chans(in);
        if (int(lines_.size()) != ch) lines_.assign(std::size_t(ch), {});
        for (auto& l : lines_) l.prepare(kLen);
        buf_.resize(std::size_t(n) * std::size_t(ch));
        int d = std::clamp(int(inputs.time.value * 48000.f), 1, kLen - 1);
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < ch; ++c) {
                float dry  = ugen_detail::at(in, i, c);
                float echo = lines_[std::size_t(c)].tick(dry, d,
                                                         inputs.feedback.value);
                buf_[std::size_t(i) * std::size_t(ch) + std::size_t(c)] =
                    dry * (1.f - inputs.wet.value) + echo * inputs.wet.value;
            }
        outputs.audio.value = AudioBuffer{buf_.data(), n, ch, in.sample_rate};
    }
private:
    std::vector<synth::DelayLine> lines_;
    std::vector<float> buf_;
};

// tanh waveshaper (stateless — channel count passes straight through).
struct ShaperNode {
    static consteval std::string_view name() { return "shaper"; }
    struct inputs {
        port<"audio", AudioBuffer> audio;
        slider<"drive", "", float, fp(0.1f), fp(20.f), fp(2.f)> drive;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        int ch = ugen_detail::chans(in);
        std::size_t samples = in.data ? std::size_t(in.frames) * std::size_t(ch) : 0;
        buf_.resize(samples);
        float d = inputs.drive.value, norm = std::tanh(d);
        for (std::size_t i = 0; i < samples; ++i)
            buf_[i] = std::tanh(in.data[i] * d) / norm;
        outputs.audio.value = AudioBuffer{buf_.data(), in.data ? in.frames : 0,
                                          ch, in.sample_rate};
    }
private:
    std::vector<float> buf_;
};

// Sample & hold, per-channel held values on a shared clock.
struct SampleHoldNode {
    static consteval std::string_view name() { return "sample_hold"; }
    struct inputs {
        port<"audio", AudioBuffer> audio;
        slider<"rate", "Hz", float, fp(0.5f), fp(2000.f), fp(20.f)> rate;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        int n  = in.data ? in.frames : 0;
        int ch = ugen_detail::chans(in);
        if (int(held_.size()) != ch) held_.assign(std::size_t(ch), {});
        buf_.resize(std::size_t(n) * std::size_t(ch));
        float period = 48000.f / std::max(0.5f, inputs.rate.value);
        for (int i = 0; i < n; ++i) {
            bool sample = (++count_ >= period);
            if (sample) count_ = 0.f;
            for (int c = 0; c < ch; ++c)
                buf_[std::size_t(i) * std::size_t(ch) + std::size_t(c)] =
                    held_[std::size_t(c)].tick(ugen_detail::at(in, i, c), sample);
        }
        outputs.audio.value = AudioBuffer{buf_.data(), n, ch, in.sample_rate};
    }
private:
    std::vector<float> buf_;
    std::vector<synth::SampleHold> held_;
    float count_ = 0.f;
};

// Scalar slew limiter: de-zippers latched params (units per second).
struct SlewNode {
    static consteval std::string_view name() { return "slew"; }
    struct inputs {
        slider<"target", "", float, fp(-20000.f), fp(20000.f), fp(0.f)>   target;
        slider<"rate", "/s", float, fp(0.01f),    fp(100000.f), fp(10.f)> rate;
    } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double t) {
        double dt = (prev_t_ > 0.0) ? t - prev_t_ : 1.0 / 60.0;
        prev_t_ = t;
        if (dt <= 0.0 || dt > 0.1) dt = 1.0 / 60.0;
        outputs.out.value = k_.tick(inputs.target.value,
                                    inputs.rate.value * float(dt));
    }
private:
    double      prev_t_ = 0.0;
    synth::Slew k_;
};

// Bang clock: fires `rate` times per second. `gate_out` is the audio-rate
// square gate (width × duty) for the CV adsr.
struct MetroNode {
    static consteval std::string_view name() { return "metro"; }
    struct inputs {
        slider<"rate", "/s", float, fp(0.f), fp(50.f), fp(1.f)> rate;
        slider<"duty", "",   float, fp(0.f), fp(1.f),  fp(0.5f)> duty;
    } inputs;
    struct outputs {
        port<"bang_out", float>       bang_out;
        bang<"fire">                  fire;
        port<"gate_out", AudioBuffer> gate_out;
    } outputs;
    void operator()(double t) {
        outputs.fire.triggered = false;
        outputs.bang_out.value = 0.f;
        int n = g_.frames(t);
        float r = inputs.rate.value;
        if (r > 0.f) {
            // next_ can legitimately be at most one period ahead. Further
            // means t's EPOCH changed under us — a region migration moved
            // this instance between frame time (XR clock, ~1e5 s) and
            // block time (stream clock, ~minutes) — and the metro would
            // otherwise never fire again (bricked-chime bug, 2026-06-12).
            if (next_ == 0.0 || next_ > t + 1.0 / double(r)) {
                next_       = t;
                gate_until_ = -1.0;
            }
            if (t >= next_) {
                outputs.fire.triggered = true;
                outputs.bang_out.value = 1.f;
                gate_until_ = next_ + double(inputs.duty.value) / double(r);
                next_ += 1.0 / double(r);
                if (t >= next_) next_ = t + 1.0 / double(r);
            }
        }
        for (int i = 0; i < n; ++i) {
            double ts = t - double(n - i) / 48000.0;
            g_.buf[std::size_t(i)] = (r > 0.f && ts < gate_until_) ? 1.f : 0.f;
        }
        outputs.gate_out.value = AudioBuffer{g_.buf.data(), n, 1, 48000};
    }
private:
    ugen_detail::Gen g_;
    double next_ = 0.0, gate_until_ = -1.0;
};

// Concatenate up to four sources' channel lists into one multichannel
// edge (Max's mc.pack~; Pd-style channel concatenation).
struct McPackNode {
    static consteval std::string_view name() { return "mc_pack"; }
    struct inputs {
        port<"in1", AudioBuffer> in1;
        port<"in2", AudioBuffer> in2;
        port<"in3", AudioBuffer> in3;
        port<"in4", AudioBuffer> in4;
    } inputs;
    struct outputs { port<"audio", AudioBuffer> audio; } outputs;
    void operator()(double) {
        const AudioBuffer* ins[4] = {&inputs.in1.value, &inputs.in2.value,
                                     &inputs.in3.value, &inputs.in4.value};
        int n = 0, ch = 0, rate = 48000;
        for (auto* b : ins)
            if (b->data && b->frames > 0) {
                n    = std::max(n, b->frames);
                ch  += ugen_detail::chans(*b);
                rate = b->sample_rate;
            }
        int out_ch = std::max(1, ch);
        buf_.assign(std::size_t(n) * std::size_t(out_ch), 0.f);
        int base = 0;
        for (auto* b : ins) {
            if (!b->data || b->frames <= 0) continue;
            int bch = ugen_detail::chans(*b);
            for (int i = 0; i < std::min(n, b->frames); ++i)
                for (int c = 0; c < bch; ++c)
                    buf_[std::size_t(i) * std::size_t(out_ch) +
                         std::size_t(base + c)] = ugen_detail::at(*b, i, c);
            base += bch;
        }
        outputs.audio.value = AudioBuffer{buf_.data(), n, out_ch, rate};
    }
private:
    std::vector<float> buf_;
};

// Peel the first four channels off a multichannel edge as mono outputs.
struct McUnpackNode {
    static consteval std::string_view name() { return "mc_unpack"; }
    struct inputs { port<"audio", AudioBuffer> audio; } inputs;
    struct outputs {
        port<"out1", AudioBuffer> out1;
        port<"out2", AudioBuffer> out2;
        port<"out3", AudioBuffer> out3;
        port<"out4", AudioBuffer> out4;
    } outputs;
    void operator()(double) {
        const AudioBuffer& in = inputs.audio.value;
        int n  = in.data ? in.frames : 0;
        int ch = ugen_detail::chans(in);
        auto peel = [&](auto& out, int c) {
            auto& buf = bufs_[std::size_t(c)];
            if (c < ch && n > 0) {
                buf.resize(std::size_t(n));
                for (int i = 0; i < n; ++i)
                    buf[std::size_t(i)] = ugen_detail::at(in, i, c);
                out.value = AudioBuffer{buf.data(), n, 1, in.sample_rate};
            } else {
                out.value = AudioBuffer{};
            }
        };
        peel(outputs.out1, 0);
        peel(outputs.out2, 1);
        peel(outputs.out3, 2);
        peel(outputs.out4, 3);
    }
private:
    std::vector<float> bufs_[4];
};

// Stochastic grain generator (the decomposition floor under rain/fire).
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
                float r = synth::white_noise(rng_);
                voices_[next_voice_++ % kVoices].strike(
                    inputs.freq.value * (1.f + inputs.spread.value * r * 0.8f));
            }
            float s = 0.f;
            for (auto& v : voices_)
                s += v.tick(inputs.texture.value, k, rng_,
                            float(ugen_detail::kRate));
            g_.buf[std::size_t(i)] = inputs.amp.value * s * 0.5f;
        }
        outputs.audio.value = AudioBuffer{g_.buf.data(), n, 1, 48000};
    }
private:
    static constexpr int kVoices = 16;
    ugen_detail::Gen g_;
    synth::GrainVoice voices_[kVoices];
    int next_voice_ = 0;
    uint32_t rng_ = 0xBEEF5EEDu;
};
