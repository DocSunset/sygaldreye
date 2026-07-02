// Copyright 2026 Travis West
#pragma once
#include <algorithm>
#include <cmath>
#include <string_view>
#include <vector>

#include "biquad_filter.hpp"
#include "block_shell.hpp"
#include "kernels.hpp"
#include "sygaldry_endpoints.hpp"
#include "synth_core.hpp"

// The audio unit generators (ugen_decomposition.md phase 1+).
//
// MULTICHANNEL, Pd-style: an audio edge carries any number of PLANAR
// (channel-major) channels, decided by what's wired (AudioBuffer.channels).
// Processors adapt transparently — per-channel state grows to match the
// input; a mono side-input broadcasts; mismatched counts wrap (c % channels).
// Generators are mono; mc_pack concatenates channel lists and mc_unpack
// peels them when explicit decomposition is wanted.
//
// Generators produce the elapsed tick's worth of samples (dt-driven);
// processors produce exactly their input's frame count.

namespace ugen_detail {
using synth::Gen;  // generator block shell (synth_core/block_shell.hpp)

inline int chans(const AudioBuffer& b) { return std::max(1, b.channels); }
// Sample of channel c, wrapping into the buffer's own channel count
// (mono broadcasts, mismatches wrap). PLANAR: each channel is contiguous,
// so channel c starts at c*frames.
inline float at(const AudioBuffer& b, int frame, int c) {
    return b.data[std::size_t(c % chans(b)) * std::size_t(b.frames) + std::size_t(frame)];
}
// Sample rate of a buffer, defaulting when the producer left it unset.
inline float rate(const AudioBuffer& b) {
    return b.sample_rate > 0 ? float(b.sample_rate) : 48000.f;
}

// The conformability stamp (conformability.md): one kernel instance per
// channel (map axis), each scanning its frames (time axis). prepare runs
// once per kernel per block (coefficient pushes), fresh=true only for
// kernels created this block; tick is the per-sample kernel call. This
// replaces every hand-written processor shell.
template <typename K>
struct Lift {
    std::vector<K> kernels;
    std::vector<float> buf;
    template <typename Prepare, typename Tick>
    AudioBuffer run(const AudioBuffer& in, Prepare&& prepare, Tick&& tick) {
        int n = (in.data && in.frames > 0) ? in.frames : 0;
        int ch = chans(in);
        // Grow/shrink, never wipe: surviving kernels keep their state
        // (delay tails, envelopes) across channel-count changes; only new
        // kernels are fresh. resize keeps capacity, so shrink/regrow under
        // the high-water mark constructs nothing.
        std::size_t survivors = std::min(kernels.size(), std::size_t(ch));
        kernels.resize(std::size_t(ch));
        for (std::size_t k = 0; k < kernels.size(); ++k) prepare(kernels[k], k >= survivors);
        buf.resize(std::size_t(n) * std::size_t(ch));  // allocates only past capacity
        for (int c = 0; c < ch; ++c)
            for (int i = 0; i < n; ++i)  // planar: scan channel c's frames
                buf[std::size_t(c) * std::size_t(n) + std::size_t(i)] =
                    tick(kernels[std::size_t(c)], at(in, i, c), i, c);
        return {buf.data(), n, ch, in.sample_rate};
    }
};
}  // namespace ugen_detail

// White (color 0) → pink (color 1) noise source. Mono.
struct NoiseNode {
    static consteval std::string_view name() { return "noise"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> amp;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> color;
        out<AudioBuffer> audio;
    } endpoints;
    void operator()(double t) {
        float a = endpoints.amp.get(), c = endpoints.color.get();
        endpoints.audio.value = g_.generate(t, [&] { return a * k_.tick(c); });
    }

private:
    ugen_detail::Gen g_;
    synth::PinkNoise k_;
};

// Percussive one-shot envelope: bang (or gate rise) → attack/decay,
// auto-release. The pluck/strike envelope; for held notes use `adsr`.
struct PercNode {
    static consteval std::string_view name() { return "perc"; }
    struct endpoints {
        event_in trigger;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> gate;
        normalled_in<float, fp(0.001f), fp(2.f), fp(0.01f)> attack;
        normalled_in<float, fp(0.001f), fp(2.f), fp(0.1f)> decay;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.7f)> sustain;
        normalled_in<float, fp(0.001f), fp(4.f), fp(0.2f)> release;
        out<AudioBuffer> audio;
        out<float> level;
    } endpoints;
    void operator()(double t) {
        env_.attack_s = endpoints.attack.get();
        env_.decay_s = endpoints.decay.get();
        env_.sustain_level = endpoints.sustain.get();
        env_.release_s = endpoints.release.get();
        bool gate = endpoints.gate.get() > 0.5f;
        bool trig = endpoints.trigger.triggered;
        if (trig || (gate && !prev_gate_)) env_.gate_on();
        if (!gate && prev_gate_ && !trig) env_.gate_off();
        if (trig && !gate) auto_release_ = true;
        prev_gate_ = gate;
        endpoints.audio.value = g_.generate(t, [&] { return env_.tick(); });
        if (auto_release_ && env_.stage == synth::Adsr::Stage::Sustain) {
            env_.gate_off();
            auto_release_ = false;
        }
        endpoints.level.value = env_.envelope;
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
    // CV-style: the gate IS an audio signal (threshold 0.5); per-channel
    // envelopes — a multichannel gate yields a multichannel envelope.
    struct endpoints {
        in<AudioBuffer> gate;
        normalled_in<float, fp(0.001f), fp(2.f), fp(0.01f)> attack;
        normalled_in<float, fp(0.001f), fp(2.f), fp(0.1f)> decay;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.7f)> sustain;
        normalled_in<float, fp(0.001f), fp(4.f), fp(0.2f)> release;
        out<AudioBuffer> audio_out;
    } endpoints;
    struct GateEnv {
        synth::Adsr env;
        bool prev = false;
    };
    void operator()(double) {
        const AudioBuffer in = endpoints.gate.get();
        float sr = ugen_detail::rate(in);
        float a = endpoints.attack.get(), d = endpoints.decay.get();
        float su = endpoints.sustain.get(), r = endpoints.release.get();
        endpoints.audio_out.value = lift_.run(
            in,
            [&](GateEnv& k, bool) {
                k.env.sample_rate = sr;
                k.env.attack_s = a;
                k.env.decay_s = d;
                k.env.sustain_level = su;
                k.env.release_s = r;
            },
            [](GateEnv& k, float x, int, int) {
                bool g = x > 0.5f;
                if (g && !k.prev) k.env.gate_on();
                if (!g && k.prev) k.env.gate_off();
                k.prev = g;
                return k.env.tick();
            });
    }

private:
    ugen_detail::Lift<GateEnv> lift_;
};

// Multiply audio by a scalar gain and (optionally) an audio-rate envelope
// (mono env broadcasts; otherwise channel-wise).
struct VcaNode {
    static consteval std::string_view name() { return "vca"; }
    struct endpoints {
        in<AudioBuffer> audio;
        in<AudioBuffer> env;  // mono broadcasts; otherwise channel-wise
        normalled_in<float, fp(0.f), fp(4.f), fp(1.f)> gain;
        out<AudioBuffer> audio_out;
    } endpoints;
    struct Stateless {};
    void operator()(double) {
        const AudioBuffer e = endpoints.env.get();
        float gain = endpoints.gain.get();
        endpoints.audio_out.value = lift_.run(
            endpoints.audio.get(),
            [](Stateless&, bool) {},
            [&](Stateless&, float x, int i, int c) {
                float g = gain;
                if (e.data && e.frames > 0) g *= ugen_detail::at(e, std::min(i, e.frames - 1), c);
                return x * g;
            });
    }

private:
    ugen_detail::Lift<Stateless> lift_;
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
        out<AudioBuffer> audio_out;
    } endpoints;
    void operator()(double) {
        const AudioBuffer bufs[4] = {
            endpoints.in1.get(), endpoints.in2.get(), endpoints.in3.get(), endpoints.in4.get()};
        float gains[4] = {
            endpoints.g1.get(), endpoints.g2.get(), endpoints.g3.get(), endpoints.g4.get()};
        int n = 0, ch = 1, rate = 48000;
        for (const auto& b : bufs)
            if (b.data && b.frames > 0) {
                n = std::max(n, b.frames);
                ch = std::max(ch, ugen_detail::chans(b));
                rate = b.sample_rate;
            }
        buf_.assign(std::size_t(n) * std::size_t(ch), 0.f);
        for (int s = 0; s < 4; ++s) {
            if (!bufs[s].data) continue;
            for (int c = 0; c < ch; ++c)
                for (int i = 0; i < std::min(n, bufs[s].frames); ++i)
                    buf_[std::size_t(c) * std::size_t(n) + std::size_t(i)] +=
                        ugen_detail::at(bufs[s], i, c) * gains[s];
        }
        endpoints.audio_out.value = AudioBuffer{buf_.data(), n, ch, rate};
    }

private:
    std::vector<float> buf_;
};

// Biquad filter, per-channel state. mode: 0 LP, 1 HP, 2 BP.
struct BiquadNode {
    static consteval std::string_view name() { return "biquad"; }
    // v6 + lifted: the kernel is synth::BiquadFilter; the channel/frame
    // plumbing is the Lift stamp. mode: 0 LP, 1 HP, 2 BP.
    struct endpoints {
        in<AudioBuffer> audio;
        normalled_in<float, fp(20.f), fp(20000.f), fp(1000.f)> freq;
        normalled_in<float, fp(0.3f), fp(20.f), fp(0.707f)> q;
        normalled_in<float, fp(0.f), fp(2.f), fp(0.f)> mode;
        out<AudioBuffer> audio_out;
    } endpoints;
    void operator()(double) {
        const AudioBuffer in = endpoints.audio.get();
        float sr = ugen_detail::rate(in);
        float fr = endpoints.freq.get(), qq = endpoints.q.get();
        int m = int(endpoints.mode.get() + 0.5f);
        bool changed = fr != freq_ || qq != q_ || m != mode_ || sr != sr_;
        if (changed) {
            freq_ = fr;
            q_ = qq;
            mode_ = m;
            sr_ = sr;
            co_ = m == 1   ? synth::high_pass(fr, qq, sr)
                  : m == 2 ? synth::band_pass(fr, qq, sr)
                           : synth::low_pass(fr, qq, sr);
        }
        endpoints.audio_out.value = lift_.run(
            in,
            [&](synth::BiquadFilter& k, bool fresh) {
                if (changed || fresh) k.set_coeffs(co_);
            },
            [](synth::BiquadFilter& k, float x, int, int) { return k.tick(x); });
    }

private:
    ugen_detail::Lift<synth::BiquadFilter> lift_;
    synth::BiquadCoeffs co_{};
    float freq_ = -1.f, q_ = -1.f, sr_ = -1.f;
    int mode_ = -1;
};

// Sample-accurate delay with feedback, per-channel lines.
struct DelayNode {
    static consteval std::string_view name() { return "delay"; }
    struct endpoints {
        in<AudioBuffer> audio;
        normalled_in<float, fp(0.001f), fp(2.f), fp(0.25f)> time;
        normalled_in<float, fp(0.f), fp(0.98f), fp(0.4f)> feedback;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> wet;
        out<AudioBuffer> audio_out;
    } endpoints;
    static constexpr int kLen = 96000;  // fixed max: never resizes on sr change
    void operator()(double) {
        const AudioBuffer in = endpoints.audio.get();
        int d = std::clamp(int(endpoints.time.get() * ugen_detail::rate(in)), 1, kLen - 1);
        float fb = endpoints.feedback.get(), wet = endpoints.wet.get();
        endpoints.audio_out.value = lift_.run(
            in,
            [](synth::DelayLine& k, bool) { k.prepare(kLen); },
            [&](synth::DelayLine& k, float x, int, int) {
                float echo = k.tick(x, d, fb);
                return x * (1.f - wet) + echo * wet;
            });
    }

private:
    ugen_detail::Lift<synth::DelayLine> lift_;
};

// tanh waveshaper (stateless — channel count passes straight through).
struct ShaperNode {
    static consteval std::string_view name() { return "shaper"; }
    struct endpoints {
        in<AudioBuffer> audio;
        normalled_in<float, fp(0.1f), fp(20.f), fp(2.f)> drive;
        out<AudioBuffer> audio_out;
    } endpoints;
    struct Stateless {};
    void operator()(double) {
        float d = endpoints.drive.get(), norm = std::tanh(d);
        endpoints.audio_out.value = lift_.run(
            endpoints.audio.get(),
            [](Stateless&, bool) {},
            [&](Stateless&, float x, int, int) { return std::tanh(x * d) / norm; });
    }

private:
    ugen_detail::Lift<Stateless> lift_;
};

// Sample & hold, per-channel held values on a shared clock.
struct SampleHoldNode {
    static consteval std::string_view name() { return "sample_hold"; }
    struct endpoints {
        in<AudioBuffer> audio;
        normalled_in<float, fp(0.5f), fp(2000.f), fp(20.f)> rate;
        out<AudioBuffer> audio_out;
    } endpoints;
    void operator()(double) {
        const AudioBuffer in = endpoints.audio.get();
        int n = (in.data && in.frames > 0) ? in.frames : 0;
        float period = ugen_detail::rate(in) / std::max(0.5f, endpoints.rate.get());
        // The clock is a time-axis scan shared by every channel: compute the
        // per-frame sample mask once, then map S&H across channels (planar
        // channel-outer iteration must not reach across channels per frame).
        mask_.resize(std::size_t(n));
        for (int i = 0; i < n; ++i) {
            bool s = (++count_ >= period);
            if (s) count_ = 0.f;
            mask_[std::size_t(i)] = s ? 1 : 0;
        }
        endpoints.audio_out.value = lift_.run(
            in,
            [](synth::SampleHold&, bool) {},
            [&](synth::SampleHold& k, float x, int i, int) {
                return k.tick(x, mask_[std::size_t(i)] != 0);
            });
    }

private:
    ugen_detail::Lift<synth::SampleHold> lift_;
    std::vector<char> mask_;
    float count_ = 0.f;
};

// Scalar slew limiter: de-zippers latched params (units per second).
struct SlewNode {
    static consteval std::string_view name() { return "slew"; }
    struct endpoints {
        normalled_in<float, fp(-20000.f), fp(20000.f), fp(0.f)> target;
        normalled_in<float, fp(0.01f), fp(100000.f), fp(10.f)> rate;
        ::out<float> out;  // ::-qualified: the field name shadows the shape
    } endpoints;
    void operator()(double t) {
        double dt = (prev_t_ > 0.0) ? t - prev_t_ : 1.0 / 60.0;
        prev_t_ = t;
        if (dt <= 0.0 || dt > 0.1) dt = 1.0 / 60.0;
        endpoints.out.value = k_.tick(endpoints.target.get(), endpoints.rate.get() * float(dt));
    }

private:
    double prev_t_ = 0.0;
    synth::Slew k_;
};

// Bang clock: fires `rate` times per second. `gate_out` is the audio-rate
// square gate (width × duty) for the CV adsr.
//
// KERNEL-EXTRACTION EXCEPTION (kernel_extraction.md): metro is a time/event
// node, not a per-sample DSP kernel — it derives bang/event_out and a gate
// whose edges fall at specific per-sample timestamps within the block. It
// uses Gen::frames() for sizing but writes the gate buffer by hand rather
// than through generate(): there is no per-sample kernel to lift.
struct MetroNode {
    static consteval std::string_view name() { return "metro"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(50.f), fp(1.f)> rate;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> duty;
        out<float> bang_out;
        event_out fire;
        out<AudioBuffer> gate_out;
    } endpoints;
    void operator()(double t) {
        endpoints.fire.triggered = false;
        endpoints.bang_out.value = 0.f;
        int n = g_.frames(t);
        float r = endpoints.rate.get();
        if (r > 0.f) {
            // next_ can legitimately be at most one period ahead. Further
            // means t's EPOCH changed under us (region migration between
            // XR and stream clocks) — resync or never fire again.
            if (next_ == 0.0 || next_ > t + 1.0 / double(r)) {
                next_ = t;
                gate_until_ = -1.0;
            }
            if (t >= next_) {
                endpoints.fire.triggered = true;
                endpoints.bang_out.value = 1.f;
                gate_until_ = next_ + double(endpoints.duty.get()) / double(r);
                next_ += 1.0 / double(r);
                if (t >= next_) next_ = t + 1.0 / double(r);
            }
        }
        for (int i = 0; i < n; ++i) {
            double ts = t - double(n - i) / 48000.0;
            g_.buf[std::size_t(i)] = (r > 0.f && ts < gate_until_) ? 1.f : 0.f;
        }
        endpoints.gate_out.value = AudioBuffer{g_.buf.data(), n, 1, 48000};
    }

private:
    ugen_detail::Gen g_;
    double next_ = 0.0, gate_until_ = -1.0;
};

// Stochastic grain generator (the decomposition floor under rain/fire).
struct GrainCloudNode {
    static consteval std::string_view name() { return "grain_cloud"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(2000.f), fp(100.f)> rate;
        normalled_in<float, fp(50.f), fp(12000.f), fp(3000.f)> freq;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> spread;
        normalled_in<float, fp(0.001f), fp(0.5f), fp(0.02f)> decay;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> texture;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> amp;
        out<AudioBuffer> audio;
    } endpoints;
    void operator()(double t) {
        float p_spawn = endpoints.rate.get() / synth::kSampleRate;
        float k = 1.f - 1.f / (endpoints.decay.get() * synth::kSampleRate);
        float freq = endpoints.freq.get(), spread = endpoints.spread.get();
        float texture = endpoints.texture.get(), amp = endpoints.amp.get();
        endpoints.audio.value = g_.generate(t, [&] {
            if (synth::white_noise(rng_) * 0.5f + 0.5f < p_spawn) {
                float r = synth::white_noise(rng_);
                voices_[next_voice_++ % kVoices].strike(freq * (1.f + spread * r * 0.8f));
            }
            float sum = 0.f;
            for (auto& v : voices_) sum += v.tick(texture, k, rng_, synth::kSampleRate);
            return amp * sum * 0.5f;
        });
    }

private:
    static constexpr int kVoices = 16;
    ugen_detail::Gen g_;
    synth::GrainVoice voices_[kVoices];
    uint32_t rng_ = 0x2468ace1u;
    int next_voice_ = 0;
};
// Concatenate up to four sources' channel lists into one multichannel
// edge (Max's mc.pack~; Pd-style channel concatenation).
struct McPackNode {
    static consteval std::string_view name() { return "mc_pack"; }
    struct endpoints {
        in<AudioBuffer> in1, in2, in3, in4;
        out<AudioBuffer> audio_out;
    } endpoints;
    void operator()(double) {
        const AudioBuffer bufs[4] = {
            endpoints.in1.get(), endpoints.in2.get(), endpoints.in3.get(), endpoints.in4.get()};
        const AudioBuffer* ins[4] = {&bufs[0], &bufs[1], &bufs[2], &bufs[3]};
        int n = 0, ch = 0, rate = 48000;
        for (auto* b : ins)
            if (b->data && b->frames > 0) {
                n = std::max(n, b->frames);
                ch += ugen_detail::chans(*b);
                rate = b->sample_rate;
            }
        int out_ch = std::max(1, ch);
        buf_.assign(std::size_t(n) * std::size_t(out_ch), 0.f);
        int base = 0;  // planar: each input's channels become contiguous blocks
        for (auto* b : ins) {
            if (!b->data || b->frames <= 0) continue;
            int bch = ugen_detail::chans(*b);
            for (int c = 0; c < bch; ++c)
                for (int i = 0; i < std::min(n, b->frames); ++i)
                    buf_[std::size_t(base + c) * std::size_t(n) + std::size_t(i)] =
                        ugen_detail::at(*b, i, c);
            base += bch;
        }
        endpoints.audio_out.value = AudioBuffer{buf_.data(), n, out_ch, rate};
    }

private:
    std::vector<float> buf_;
};

// Peel the first four channels off a multichannel edge as mono outputs.
struct McUnpackNode {
    static consteval std::string_view name() { return "mc_unpack"; }
    struct endpoints {
        in<AudioBuffer> audio;
        out<AudioBuffer> out1, out2, out3, out4;
    } endpoints;
    void operator()(double) {
        const AudioBuffer in = endpoints.audio.get();
        int n = in.data ? in.frames : 0;
        int ch = ugen_detail::chans(in);
        auto peel = [&](auto& out, int c) {
            auto& buf = bufs_[std::size_t(c)];
            if (c < ch && n > 0) {
                buf.resize(std::size_t(n));
                for (int i = 0; i < n; ++i) buf[std::size_t(i)] = ugen_detail::at(in, i, c);
                out.value = AudioBuffer{buf.data(), n, 1, in.sample_rate};
            } else {
                out.value = AudioBuffer{};
            }
        };
        peel(endpoints.out1, 0);
        peel(endpoints.out2, 1);
        peel(endpoints.out3, 2);
        peel(endpoints.out4, 3);
    }

private:
    std::vector<float> bufs_[4];
};
