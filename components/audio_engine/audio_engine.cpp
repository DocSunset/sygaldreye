// Copyright 2026 Travis West
#include "audio_engine.hpp"
#include <algorithm>
#include <cstring>

#ifdef __ANDROID__
#include "mic_capture.hpp"
#endif

namespace {
constexpr std::size_t kInputRingCap = 48000 * 2;  // 2 s of mono input
}

struct AudioEngine::InputState {
#ifdef __ANDROID__
    std::optional<MicCapture> mic;
#endif
    std::vector<float>  ring  = std::vector<float>(kInputRingCap);
    std::atomic<size_t> head{0}, tail{0};
    std::vector<float>  scratch = std::vector<float>(4800);
};

AudioEngine& AudioEngine::instance() {
    static AudioEngine e;
    return e;
}

void AudioEngine::ensure_output(AudioCallback render) {
    std::lock_guard<std::mutex> lock(setup_mutex_);
    render_ = std::move(render);
    if (!output_ && enable_device)
        if ((output_ = AudioOutput::create(
                 [this](float* out, int frames) { callback(out, frames); },
                 kRate)))
            output_->start();
}

void AudioEngine::recover_if_dead() {
    std::lock_guard<std::mutex> lock(setup_mutex_);
    if (!output_ || !output_->dead()) return;
    output_.reset();
    if (enable_device && render_)
        if ((output_ = AudioOutput::create(
                 [this](float* out, int frames) { callback(out, frames); },
                 kRate)))
            output_->start();
}

void AudioEngine::add_input_tap() {
    std::lock_guard<std::mutex> lock(setup_mutex_);
    if (input_taps_++ == 0) open_input_locked();
}

void AudioEngine::remove_input_tap() {
    std::lock_guard<std::mutex> lock(setup_mutex_);
    if (--input_taps_ > 0) return;
    // Last tap gone: close the capture stream. Migration keeps mic node
    // instances alive across ordinary swaps, so this is rare by design.
    InputState* s = input_;
    input_ = nullptr;   // audio callback sees null before stream teardown
    delete s;
}

void AudioEngine::open_input_locked() {
#ifdef __ANDROID__
    auto* s = new InputState{};
    s->mic = MicCapture::create(kRate);
    if (s->mic) {
        s->mic->start();
        input_ = s;
    } else {
        delete s;
    }
#endif
}

int AudioEngine::read_input(float* dst, int max_frames) {
    InputState* s = input_;
    if (!s || max_frames <= 0) return 0;
    size_t tail = s->tail.load(std::memory_order_relaxed);
    size_t head = s->head.load(std::memory_order_acquire);
    int n = int(std::min<size_t>(head - tail, std::size_t(max_frames)));
    for (int i = 0; i < n; ++i)
        dst[i] = s->ring[(tail + std::size_t(i)) % kInputRingCap];
    s->tail.store(tail + std::size_t(n), std::memory_order_release);
    return n;
}

void AudioEngine::play(std::vector<float> samples) {
    std::lock_guard<std::mutex> lock(play_mutex_);
    play_queue_.push_back(std::move(samples));
}

void AudioEngine::callback(float* out, int frames) {
    // 1. Drain the capture stream into the shared input ring (pull mode —
    //    this audio thread is the only reader of the device).
    if (InputState* s = input_) {
#ifdef __ANDROID__
        if (s->mic) {
            int got = s->mic->read(s->scratch.data(),
                                   int(std::min<std::size_t>(s->scratch.size(),
                                                             std::size_t(frames) * 2)));
            size_t head = s->head.load(std::memory_order_relaxed);
            size_t tail = s->tail.load(std::memory_order_acquire);
            for (int i = 0; i < got; ++i) {
                if (head - tail >= kInputRingCap) break;  // full: drop newest
                s->ring[head % kInputRingCap] = s->scratch[std::size_t(i)];
                ++head;
            }
            s->head.store(head, std::memory_order_release);
        }
#endif
    }

    // 2. The block scheduler renders the graph (memsets `out` itself).
    if (render_) render_(out, frames);
    else         std::memset(out, 0, std::size_t(frames) * 2 * sizeof(float));

    // 3. System sounds mix on top — /play works whatever the graph is.
    if (playing_.empty()) {
        if (play_mutex_.try_lock()) {
            if (!play_queue_.empty()) {
                playing_  = std::move(play_queue_.front());
                play_queue_.erase(play_queue_.begin());
                play_pos_ = 0;
            }
            play_mutex_.unlock();
        }
    }
    if (!playing_.empty()) {
        int n = int(std::min<std::size_t>(std::size_t(frames),
                                          playing_.size() - play_pos_));
        for (int i = 0; i < n; ++i) {
            float v = playing_[play_pos_ + std::size_t(i)];
            out[i * 2]     += v;
            out[i * 2 + 1] += v;
        }
        play_pos_ += std::size_t(n);
        if (play_pos_ >= playing_.size()) { playing_.clear(); play_pos_ = 0; }
    }
}
