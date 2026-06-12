// Copyright 2026 Travis West
#include "stt_whisper.hpp"
#include "eyeballs_node_abi.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <thread>

namespace {
// PCM16 mono wav → float vector + rate (test helper, minimal RIFF).
bool load_wav(const char* path, std::vector<float>& out, int& rate) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    unsigned char h[44];
    f.read(reinterpret_cast<char*>(h), 44);
    if (std::memcmp(h, "RIFF", 4) != 0) return false;
    rate = h[24] | h[25] << 8 | h[26] << 16 | h[27] << 24;
    std::vector<char> data((std::istreambuf_iterator<char>(f)),
                            std::istreambuf_iterator<char>());
    out.resize(data.size() / 2);
    for (std::size_t i = 0; i < out.size(); ++i) {
        int16_t v;
        std::memcpy(&v, &data[i * 2], 2);
        out[i] = float(v) / 32768.f;
    }
    return true;
}
} // namespace

TEST(SttWhisper, TranscribesSpokenPhrase) {
    if (!std::ifstream("assets/models/ggml-base.en.bin").good())
        GTEST_SKIP() << "model not fetched (sh/fetch_models.sh)";
    if (std::system("espeak-ng -s 130 -w /tmp/stt_test.wav "
                    "'hello world, this is a test.' 2>/dev/null") != 0)
        GTEST_SKIP() << "espeak-ng unavailable";

    std::vector<float> wav;
    int rate = 0;
    ASSERT_TRUE(load_wav("/tmp/stt_test.wav", wav, rate));
    // Linear resample to the 48 kHz the mic edge carries.
    std::vector<float> w48(wav.size() * 48000 / std::size_t(rate));
    for (std::size_t i = 0; i < w48.size(); ++i) {
        double t = double(i) * rate / 48000.0;
        std::size_t i0 = std::size_t(t);
        std::size_t i1 = std::min(i0 + 1, wav.size() - 1);
        float frac = float(t - double(i0));
        w48[i] = wav[i0] * (1.f - frac) + wav[i1] * frac;
    }

    SttWhisperNode n;
    n.inputs.model.value = "assets/models/ggml-base.en.bin";
    n.inputs.record.value = 1.f;
    // Feed in ~16 ms mic-edge chunks.
    for (std::size_t off = 0; off < w48.size(); off += 768) {
        int len = int(std::min<std::size_t>(768, w48.size() - off));
        n.inputs.audio_in.value = AudioBuffer{w48.data() + off, len, 1, 48000};
        n(0.0);
    }
    n.inputs.audio_in.value = {};
    n.inputs.record.value = 0.f;
    n(0.0);
    n.inputs.send.value = 1.f;
    n(0.0);
    n.inputs.send.value = 0.f;

    std::string text;
    for (int i = 0; i < 600; ++i) {   // up to 60 s (model load + inference)
        n(0.0);
        if (n.outputs.heard.triggered) { text = n.outputs.text.value; break; }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);
    EXPECT_NE(text.find("hello"), std::string::npos) << "got: " << text;
    EXPECT_NE(text.find("test"),  std::string::npos) << "got: " << text;
}
