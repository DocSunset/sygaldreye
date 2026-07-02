// Copyright 2026 Travis West
#include "spectrogram.hpp"
#include <algorithm>
#include <cmath>
#include <complex>

namespace {
// In-place iterative radix-2 FFT (size must be a power of two).
void fft(std::vector<std::complex<float>>& a) {
    const std::size_t n = a.size();
    for (std::size_t i = 1, j = 0; i < n; ++i) {
        std::size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (std::size_t len = 2; len <= n; len <<= 1) {
        float ang = -6.2831853f / float(len);
        std::complex<float> wl{std::cos(ang), std::sin(ang)};
        for (std::size_t i = 0; i < n; i += len) {
            std::complex<float> w{1.f, 0.f};
            for (std::size_t k = 0; k < len / 2; ++k) {
                auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k]           = u + v;
                a[i + k + len / 2] = u - v;
                w *= wl;
            }
        }
    }
}
} // namespace

SpectrogramNode::SpectrogramNode() : window_(kFft) {
    for (int i = 0; i < kFft; ++i)
        window_[std::size_t(i)] =
            0.5f - 0.5f * std::cos(6.2831853f * float(i) / float(kFft - 1));
}

void SpectrogramNode::ensure_size() {
    int want = std::max(64, int(endpoints.columns.get()));
    if (want == width_) return;
    width_ = want;
    image_.assign(std::size_t(width_) * (kFft / 2), 0);
    cursor_ = 0;
}

void SpectrogramNode::operator()(double) {
    ensure_size();
    const AudioBuffer& in = endpoints.audio.get();
    if (in.data && in.frames > 0) {
        int stride = std::max(1, in.channels);
        int ch     = std::clamp(int(endpoints.channel.get()), 0, stride - 1);
        for (int i = 0; i < in.frames; ++i)
            accum_.push_back(in.data[std::size_t(i) * std::size_t(stride) +
                                     std::size_t(ch)]);
    }

    std::vector<std::complex<float>> buf(kFft);
    while (int(accum_.size()) >= kFft) {
        for (int i = 0; i < kFft; ++i)
            buf[std::size_t(i)] = accum_[std::size_t(i)] * window_[std::size_t(i)];
        fft(buf);
        float gain = endpoints.gain.get();
        unsigned char* column = &image_[std::size_t(cursor_) * (kFft / 2)];
        for (int b = 0; b < kFft / 2; ++b) {
            float mag = std::abs(buf[std::size_t(b)]) * (2.f / kFft);
            float v   = std::log10(1.f + gain * 9.f * mag);  // 0..~1
            column[b] = static_cast<unsigned char>(std::clamp(v, 0.f, 1.f) * 255.f);
        }
        cursor_ = (cursor_ + 1) % width_;
        accum_.erase(accum_.begin(), accum_.begin() + kHop);
    }
}
