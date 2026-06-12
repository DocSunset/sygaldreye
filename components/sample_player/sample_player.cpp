// Copyright 2026 Travis West
#include "sample_player.hpp"
#include "wav_reader.hpp"
#include "worker.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

void SamplePlayerNode::operator()(double time_s) {
    if (endpoints.seq.get() != prev_seq_) {
        prev_seq_ = endpoints.seq.get();
        std::string path = endpoints.file.get();
        auto sh = sh_;
        auto load = [path, sh] {
            auto clip = std::make_shared<std::vector<float>>();
            if (!read_wav_mono48k(path, *clip)) {
                std::fprintf(stderr, "sample_player: failed to load %s\n",
                             path.c_str());
                return;
            }
            std::lock_guard<std::mutex> lock(sh->m);
            sh->incoming = std::move(clip);
        };
        if (!path.empty()) {
#ifdef __EMSCRIPTEN__
            load();  // no worker thread in single-threaded WASM; MEMFS is instant
#else
            Worker::shared().post(load);
#endif
        }
    }
    {
        std::unique_lock<std::mutex> lock(sh_->m, std::try_to_lock);
        if (lock.owns_lock() && sh_->incoming) {
            clip_ = std::move(sh_->incoming);
            pos_  = 0;
        }
    }
    if (endpoints.bip.get() > 0.f && prev_bip_ == 0.f) {
        bip_freq_   = endpoints.bip.get() > 0.75f ? 1100.f : 700.f;
        bip_frames_ = 4000;  // ~83 ms
    }
    prev_bip_ = endpoints.bip.get();

    double dt = (prev_t_ > 0.0) ? time_s - prev_t_ : 1.0 / 60.0;
    prev_t_ = time_s;
    if (dt <= 0.0 || dt > 0.1) dt = 1.0 / 60.0;
    int n = std::clamp(int(dt * 48000.0), 0, 4800);
    buf_.assign(std::size_t(n), 0.f);
    for (int i = 0; i < n; ++i) {
        float s = 0.f;
        if (clip_ && pos_ < clip_->size()) s += (*clip_)[pos_++] * endpoints.gain.get();
        if (bip_frames_ > 0) {
            float env = float(bip_frames_) / 4000.f;
            bip_phase_ += bip_freq_ / 48000.f;
            bip_phase_ -= std::floor(bip_phase_);
            s += 0.25f * env * std::sin(bip_phase_ * 6.2832f);
            --bip_frames_;
        }
        buf_[std::size_t(i)] = s;
    }
    endpoints.audio.value   = AudioBuffer{buf_.data(), n, 1, 48000};
    endpoints.playing.value = (clip_ && pos_ < clip_->size()) ? 1.f : 0.f;
}
