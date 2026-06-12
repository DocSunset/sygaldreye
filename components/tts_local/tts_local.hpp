// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

typedef struct SherpaOnnxOfflineTts SherpaOnnxOfflineTts;

// Warm in-process text-to-speech (sherpa-onnx vits; piper/melo voice
// models). message is a cold text inlet (text edges deliver it), say
// fires synthesis on the worker; the voice leaves as an AUDIO EDGE —
// a block-region source you can spatialize, delay, or watch on the
// spectrogram. Model stays loaded: no per-utterance process spawn, no
// cold model load (the python tts_cli latency Travis flagged). Linux
// host only (claude_tmux is the consumer).
class TtsLocalNode {
public:
    static consteval std::string_view name()          { return "tts_local"; }
    static consteval std::string_view source_header() { return "components/tts_local/tts_local.hpp"; }

    struct inputs {
        ::text<"message">   message;
        bang<"say">         say;
        // default → assets/models/vits-piper-en_US-ryan-medium (male)
        ::text<"model_dir"> model_dir;
        slider<"speed", "", float, fp(0.5f), fp(2.f),  fp(1.f)> speed;
        slider<"sid",   "", float, fp(0.f),  fp(100.f), fp(0.f)> sid;
        slider<"seq", "", float, fp(0.f), fp(1e9f), fp(0.f)> seq;  // deprecated firing path
    } inputs;

    struct outputs {
        port<"audio",    AudioBuffer> audio;
        port<"speaking", float>       speaking;
    } outputs;

    TtsLocalNode() = default;
    ~TtsLocalNode();
    TtsLocalNode(const TtsLocalNode&) = delete;
    TtsLocalNode& operator=(const TtsLocalNode&) = delete;

    void operator()(double time_s);

private:
    struct Shared {
        std::mutex                  m;
        std::vector<float>          pending;  // 48 kHz mono, appended by worker
        const SherpaOnnxOfflineTts* tts = nullptr;  // lazy, warm thereafter
    };
    std::shared_ptr<Shared> sh_ = std::make_shared<Shared>();
    std::vector<float> buf_;             // this tick's output frames
    std::vector<float> queue_;           // drained pending awaiting pacing
    std::size_t        queue_pos_ = 0;
    double             prev_t_ = 0.0;
    float              prev_seq_ = 0.f;
};
