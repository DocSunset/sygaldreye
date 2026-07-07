// Copyright 2026 Travis West
#include "tts_local.hpp"
#include "stt_whisper.hpp"
#include "eyeballs_node_abi.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <thread>

// The full-circle gate: speak with tts_local, listen with stt_whisper.
// If the machine can hear itself say "hello world", the voice loop is
// real end to end — no human ears required.
TEST(TtsLocal, WhisperHearsWhatItSays) {
    if (!std::ifstream("assets/models/vits-piper-en_US-ryan-medium/tokens.txt").good())
        GTEST_SKIP() << "voice model not fetched (sh/fetch_models.sh)";
    if (!std::ifstream("assets/models/ggml-base.en.bin").good())
        GTEST_SKIP() << "whisper model not fetched";

    TtsLocalNode tts;
    tts.endpoints.message.fallback = "hello world, the graph is speaking.";
    tts.endpoints.say.triggered = true;
    double t = 1.0;
    tts(t);
    tts.endpoints.say.triggered = false;

    // Collect paced output until the utterance ends (or 60 s timeout).
    std::vector<float> spoken;
    bool started = false;
    for (int i = 0; i < 3600; ++i) {
        t += 1.0 / 60.0;
        tts(t);
        const AudioBuffer& a = tts.endpoints.audio.value;
        if (a.data && a.frames > 0)
            spoken.insert(spoken.end(), a.data, a.data + a.frames);
        if (tts.endpoints.speaking.value > 0.5f) started = true;
        else if (started) break;
        if (!started) std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    float peak = 0.f;
    for (float s : spoken) peak = std::max(peak, std::abs(s));
    ASSERT_GT(peak, 0.05f) << "tts produced silence";
    ASSERT_GT(spoken.size(), std::size_t(48000)) << "utterance under 1 s";

    SttWhisperNode stt;
    AudioBuffer audio_src{};
    stt.endpoints.model.fallback = "assets/models/ggml-base.en.bin";
    stt.endpoints.record.fallback = 1.f;
    for (std::size_t off = 0; off < spoken.size(); off += 768) {
        int len = int(std::min<std::size_t>(768, spoken.size() - off));
        audio_src = AudioBuffer{spoken.data() + off, len, 1, 48000}; stt.endpoints.audio_in.src = &audio_src;
        stt(0.0);
    }
    audio_src = {}; stt.endpoints.audio_in.src = &audio_src;
    stt.endpoints.record.fallback = 0.f;
    stt(0.0);
    stt.endpoints.send.fallback = 1.f;
    stt(0.0);
    stt.endpoints.send.fallback = 0.f;

    std::string text;
    for (int i = 0; i < 600; ++i) {
        stt(0.0);
        if (stt.endpoints.heard.triggered) { text = stt.endpoints.text.value; break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);
    EXPECT_NE(text.find("hello world"), std::string::npos) << "heard: " << text;
    EXPECT_NE(text.find("graph"),       std::string::npos) << "heard: " << text;
}
