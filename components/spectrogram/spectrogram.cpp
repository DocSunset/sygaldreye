// Copyright 2026 Travis West
#include "spectrogram.hpp"
#include <GLES3/gl3.h>
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

SpectrogramNode::SpectrogramNode() : window_(kFft), column_(kFft / 2) {
    for (int i = 0; i < kFft; ++i)
        window_[std::size_t(i)] =
            0.5f - 0.5f * std::cos(6.2831853f * float(i) / float(kFft - 1));
}

SpectrogramNode::~SpectrogramNode() {
    // GL object leak on destruction off the render thread is the lesser
    // evil; graphs are destroyed render-side in practice.
    if (tex_) glDeleteTextures(1, &tex_);
}

void SpectrogramNode::ensure_gl() {
    int want = std::max(64, int(inputs.columns.value));
    if (tex_ && want == width_) return;
    if (tex_) glDeleteTextures(1, &tex_);
    width_ = want;
    glGenTextures(1, &tex_);
    glBindTexture(GL_TEXTURE_2D, tex_);
    std::vector<unsigned char> zero(std::size_t(width_) * (kFft / 2), 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width_, kFft / 2, 0,
                 GL_RED, GL_UNSIGNED_BYTE, zero.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    cursor_ = 0;
}

void SpectrogramNode::operator()(double) {
    ensure_gl();
    const AudioBuffer& in = inputs.audio.value;
    if (in.data && in.frames > 0) {
        int stride = std::max(1, in.channels);
        for (int i = 0; i < in.frames; ++i)
            accum_.push_back(in.data[std::size_t(i) * std::size_t(stride)]);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, tex_);
    std::vector<std::complex<float>> buf(kFft);
    while (int(accum_.size()) >= kFft) {
        for (int i = 0; i < kFft; ++i)
            buf[std::size_t(i)] = accum_[std::size_t(i)] * window_[std::size_t(i)];
        fft(buf);
        float gain = inputs.gain.value;
        for (int b = 0; b < kFft / 2; ++b) {
            float mag = std::abs(buf[std::size_t(b)]) * (2.f / kFft);
            float v   = std::log10(1.f + gain * 9.f * mag);  // 0..~1
            column_[std::size_t(b)] =
                static_cast<unsigned char>(std::clamp(v, 0.f, 1.f) * 255.f);
        }
        glTexSubImage2D(GL_TEXTURE_2D, 0, cursor_, 0, 1, kFft / 2,
                        GL_RED, GL_UNSIGNED_BYTE, column_.data());
        cursor_ = (cursor_ + 1) % width_;
        accum_.erase(accum_.begin(), accum_.begin() + kHop);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    outputs.texture.value = GpuTexture{tex_, width_, kFft / 2, GL_R8, GL_LINEAR};
}
