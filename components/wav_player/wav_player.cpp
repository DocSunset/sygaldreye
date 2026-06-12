// Copyright 2026 Travis West
#include "wav_player.hpp"
#include "audio_engine.hpp"
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

// An engine tap: playback mixes in at the SINGLE process audio engine
// (Pd model) — this node owns no hardware. playing is a best-effort
// estimate from the clip length.
void WavPlayerNode::operator()(double time_s) {
    if (inputs.seq.value != prev_seq_) {
        prev_seq_ = inputs.seq.value;
        std::vector<float> clip;
        if (!inputs.file.value.empty() && read_wav(inputs.file.value, clip)) {
            float g = inputs.gain.value;
            if (g != 1.f) for (float& s : clip) s *= g;
            playing_until_ = time_s + double(clip.size()) / 48000.0;
            AudioEngine::instance().play(std::move(clip));
        } else if (!inputs.file.value.empty()) {
            std::fprintf(stderr, "wav_player: failed to load %s\n",
                         inputs.file.value.c_str());
        }
    }
    if (inputs.bip.value > 0.f && prev_bip_ == 0.f) {
        // Short sine bip (~83 ms), decaying envelope.
        float freq = inputs.bip.value > 0.75f ? 1100.f : 700.f;
        std::vector<float> bip(4000);
        float phase = 0.f;
        for (size_t i = 0; i < bip.size(); ++i) {
            float env = float(bip.size() - i) / float(bip.size());
            phase += freq / 48000.f;
            phase -= std::floor(phase);
            bip[i] = 0.25f * env * std::sin(phase * 6.2832f);
        }
        AudioEngine::instance().play(std::move(bip));
    }
    prev_bip_ = inputs.bip.value;

    outputs.playing.value = (time_s < playing_until_) ? 1.f : 0.f;
}
