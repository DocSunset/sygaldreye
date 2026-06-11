// Copyright 2026 Travis West
#include "wav_reader.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

bool read_wav_mono48k(const std::string& path, std::vector<float>& mono48k) {
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
            short v;
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
    if (frames == 0) return false;
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
