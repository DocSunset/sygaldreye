// Copyright 2025 Travis West
#include "mic_input.hpp"
#include <algorithm>

MicInputNode::MicInputNode() {
    mic_ = MicCapture::create([this](const float* s, int n) {
        std::lock_guard lk(mutex_);
        accum_.insert(accum_.end(), s, s + n);
    });
    if (mic_) mic_->start();
}

MicInputNode::~MicInputNode() {
    if (mic_) mic_->stop();
}

void MicInputNode::operator()(double /*time_s*/) {
    {
        std::lock_guard lk(mutex_);
        std::swap(accum_, tick_buf_);
    }
    if (tick_buf_.empty() || !mic_) {
        outputs.audio_out.value = {};
        return;
    }
    float gain = inputs.gain.value;
    for (float& s : tick_buf_) s *= gain;
    outputs.audio_out.value = {tick_buf_.data(), static_cast<int>(tick_buf_.size()), 1, 16000};
}
