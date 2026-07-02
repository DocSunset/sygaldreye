// Copyright 2026 Travis West
#pragma once
#include "synth_core.hpp"
#include <algorithm>
#include <cstdint>
#include <vector>

// Per-sample DSP kernels (planning/kernel_extraction.md). The kernel
// contract: state + tick(), params arrive as arguments or plain members,
// NO absolute-time state (time arrives as per-sample increments — the
// metro epoch bug class is structurally impossible here), no allocation
// inside tick(). Allocation is permitted only in prepare() and only on
// first touch or size change (DelayLine's line buffer — idempotent, once
// per new kernel). That first-touch alloc can still land in the audio
// callback when a live edit creates a kernel; the off-thread warm pass
// that removes it is tracked in kanban/backlog/audio_region_followups.md.
// One kernel instance per channel is the lifting unit for conformability;
// the freezer composes kernels into frozen plugins.

namespace synth {

// White→pink noise (Paul Kellet approximation), crossfaded by color.
struct PinkNoise {
    uint32_t rng = 0x1234567u;
    float p0 = 0.f, p1 = 0.f, p2 = 0.f;
    float tick(float color) {
        float w = white_noise(rng);
        p0 = 0.99765f * p0 + w * 0.0990460f;
        p1 = 0.96300f * p1 + w * 0.2965164f;
        p2 = 0.57000f * p2 + w * 1.0526913f;
        float pink = (p0 + p1 + p2 + w * 0.1848f) * 0.25f;
        return (1.f - color) * w + color * pink;
    }
};

// Feedback echo, one line per channel. prepare() sizes outside tick().
struct DelayLine {
    std::vector<float> buf;
    int write = 0;
    void prepare(int len) {
        if (int(buf.size()) != len) { buf.assign(std::size_t(len), 0.f); write = 0; }
    }
    float tick(float dry, int delay_samples, float feedback) {
        int len = int(buf.size());
        int rd  = (write - delay_samples + len) % len;
        float echo = buf[std::size_t(rd)];
        buf[std::size_t(write)] = dry + echo * feedback;
        write = (write + 1) % len;
        return echo;
    }
};

// Sample & hold: the clock lives in the caller; the kernel just holds.
struct SampleHold {
    float held = 0.f;
    float tick(float x, bool sample) {
        if (sample) held = x;
        return held;
    }
};

// Slew limiter: max_step = rate * dt, computed by the caller.
struct Slew {
    float cur = 0.f;
    float tick(float target, float max_step) {
        cur += std::clamp(target - cur, -max_step, max_step);
        return cur;
    }
};

// One grain: struck with a frequency, decays exponentially.
struct GrainVoice {
    float env = 0.f, phase = 0.f, freq = 440.f;
    void strike(float f) { env = 1.f; phase = 0.f; freq = f; }
    float tick(float texture, float decay_k, uint32_t& rng, float sample_rate) {
        if (env <= 0.001f) return 0.f;
        float tone  = sine(phase);
        float burst = white_noise(rng);
        float s = env * ((1.f - texture) * tone + texture * burst);
        phase += 6.2831853f * freq / sample_rate;
        if (phase > 6.2831853f) phase -= 6.2831853f;
        env *= decay_k;
        return s;
    }
};

} // namespace synth
