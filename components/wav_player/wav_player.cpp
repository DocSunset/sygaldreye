// Copyright 2026 Travis West
#include "wav_player.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
// Minimal RIFF reader: PCM16 or float32, any rate, mono/stereo → mono 48k.
bool read_wav(const std::string& path, std::vector<float>& mono48k) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    unsigned char h[44];
    if (std::fread(h, 1, 44, f) != 44 || std::memcmp(h, "RIFF", 4) != 0) {
        std::fclose(f);
        return false;
    }
    auto u16 = [&](int o) { return int(h[o]) | int(h[o + 1]) << 8; };
    auto u32 = [&](int o) {
        return unsigned(h[o]) | unsigned(h[o + 1]) << 8 |
               unsigned(h[o + 2]) << 16 | unsigned(h[o + 3]) << 24;
    };
    int fmt = u16(20), channels = u16(22), bits = u16(34);
    unsigned rate = u32(24), data_len = u32(40);
    std::vector<unsigned char> data(data_len);
    data_len = unsigned(std::fread(data.data(), 1, data_len, f));
    std::fclose(f);

    std::vector<float> src;
    int frames = 0;
    if (fmt == 1 && bits == 16) {
        frames = int(data_len) / (2 * channels);
        src.resize(size_t(frames));
        for (int i = 0; i < frames; ++i) {
            int16_t v;
            std::memcpy(&v, &data[size_t(i) * 2 * unsigned(channels)], 2);
            src[size_t(i)] = float(v) / 32768.f;
        }
    } else if (fmt == 3 && bits == 32) {
        frames = int(data_len) / (4 * channels);
        src.resize(size_t(frames));
        for (int i = 0; i < frames; ++i)
            std::memcpy(&src[size_t(i)], &data[size_t(i) * 4 * unsigned(channels)], 4);
    } else {
        return false;
    }
    // Linear resample to 48k.
    double ratio = 48000.0 / double(rate);
    size_t out_n = size_t(double(frames) * ratio);
    mono48k.resize(out_n);
    for (size_t i = 0; i < out_n; ++i) {
        double t = double(i) / ratio;
        size_t i0 = size_t(t), i1 = std::min(i0 + 1, size_t(frames - 1));
        float frac = float(t - double(i0));
        mono48k[i] = src[i0] * (1.f - frac) + src[i1] * frac;
    }
    return true;
}
} // namespace

void WavPlayerNode::ensure_stream() {
    if (out_) return;
    auto cb = [this](float* out, int frames) {
        float g = gain_.load(std::memory_order_relaxed);
        auto clip = std::atomic_load(&clip_);
        size_t pos = clip_pos_.load(std::memory_order_relaxed);
        float bf = bip_freq_.load(std::memory_order_relaxed);
        int   bn = bip_frames_.load(std::memory_order_relaxed);
        for (int i = 0; i < frames; ++i) {
            float s = 0.f;
            if (clip && pos < clip->size()) s += (*clip)[pos++] * g;
            if (bn > 0) {
                float env = float(bn) / 4000.f;
                bip_phase_ += bf / 48000.f;
                bip_phase_ -= std::floor(bip_phase_);
                s += 0.25f * env * std::sin(bip_phase_ * 6.2832f);
                --bn;
            }
            out[i * 2] = out[i * 2 + 1] = s;
        }
        clip_pos_.store(pos, std::memory_order_relaxed);
        bip_frames_.store(bn, std::memory_order_relaxed);
    };
    auto stream = AudioOutput::create(cb, 48000);
    if (!stream) { std::fprintf(stderr, "wav_player: stream failed\n"); return; }
    out_ = std::make_unique<AudioOutput>(std::move(*stream));
    out_->start();
}

bool WavPlayerNode::load_wav(const std::string& path) {
    auto clip = std::make_shared<std::vector<float>>();
    if (!read_wav(path, *clip)) return false;
    std::atomic_store(&clip_, clip);
    clip_pos_.store(0);
    return true;
}

void WavPlayerNode::operator()(double) {
    ensure_stream();
    gain_.store(inputs.gain.value, std::memory_order_relaxed);

    if (inputs.seq.value != prev_seq_) {
        prev_seq_ = inputs.seq.value;
        if (!inputs.file.value.empty() && !load_wav(inputs.file.value))
            std::fprintf(stderr, "wav_player: failed to load %s\n",
                         inputs.file.value.c_str());
    }
    if (inputs.bip.value > 0.f && prev_bip_ == 0.f) {
        bip_freq_.store(inputs.bip.value > 0.75f ? 1100.f : 700.f);
        bip_frames_.store(4000);  // ~83 ms
    }
    prev_bip_ = inputs.bip.value;

    auto clip = std::atomic_load(&clip_);
    outputs.playing.value =
        (clip && clip_pos_.load() < clip->size()) ? 1.f : 0.f;
}
