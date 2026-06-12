// Copyright 2025 Travis West
#include "mic_input.hpp"
#include "audio_engine.hpp"

namespace {
constexpr int kMaxTickFrames = 4800;  // 100 ms at 48 k — render hiccup slack
}

// An adc tap: the ENGINE owns the capture hardware (one stream per
// process, Pd-style); this node just drains the shared input ring.
MicInputNode::MicInputNode()  { AudioEngine::instance().add_input_tap(); }
MicInputNode::~MicInputNode() { AudioEngine::instance().remove_input_tap(); }

void MicInputNode::operator()(double /*time_s*/) {
    tick_buf_.resize(kMaxTickFrames);
    int n = AudioEngine::instance().read_input(tick_buf_.data(), kMaxTickFrames);
    if (n == 0) { outputs.audio_out.value = {}; return; }
    float gain = inputs.gain.value;
    for (int i = 0; i < n; ++i) tick_buf_[std::size_t(i)] *= gain;
    outputs.audio_out.value = {tick_buf_.data(), n, 1, 48000};
}
