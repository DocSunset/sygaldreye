// Copyright 2025 Travis West
#include "mic_input.hpp"
#include <algorithm>
#include <android/log.h>

MicInputNode::MicInputNode() = default;  // mic opens lazily on first tick

MicInputNode::~MicInputNode() {
    if (mic_) mic_->stop();
}

void MicInputNode::operator()(double /*time_s*/) {
    if (!mic_ && !mic_failed_) {  // graphs parse on the HTTP thread; open here
        mic_ = MicCapture::create([this](const float* s, int n) {
            std::lock_guard lk(mutex_);
            accum_.insert(accum_.end(), s, s + n);
        }, 48000);  // Quest mics are 48k native; 16k opens but can starve
        if (mic_) mic_->start();
        else      mic_failed_ = true;
    }
    {
        std::lock_guard lk(mutex_);
        tick_buf_.clear();  // swap alone ping-pongs ever-growing buffers
        std::swap(accum_, tick_buf_);
    }
    if (tick_buf_.empty() || !mic_) {
        // AAudio input acquisition is flaky on Quest: streams sometimes open
        // but never deliver. Watchdog: reopen after ~2 s of starvation.
        if (mic_ && ++starved_ticks_ > 144 && reopen_attempts_ < 5) {
            __android_log_print(ANDROID_LOG_WARN, "eyeballs",
                                "mic_input: starved, reopening (attempt %d)",
                                ++reopen_attempts_);
            mic_->stop();
            mic_.reset();
            starved_ticks_ = 0;  // lazy-init path recreates next tick
        }
        outputs.audio_out.value = {};
        return;
    }
    starved_ticks_ = 0;
    float gain = inputs.gain.value;
    for (float& s : tick_buf_) s *= gain;
    outputs.audio_out.value = {tick_buf_.data(), static_cast<int>(tick_buf_.size()), 1, 48000};
}
