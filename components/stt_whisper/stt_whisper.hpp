// Copyright 2026 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

struct whisper_context;

// On-device speech-to-text (whisper.cpp): hold `record` to take audio
// from the mic edge, `send` transcribes locally on the worker region,
// the transcript leaves as a TEXT EDGE (text + heard bang). Same PTT
// contract as the HTTP-relay speech_to_text node — graphs swap freely.
class SttWhisperNode {
public:
    static consteval std::string_view name()          { return "stt_whisper"; }
    static consteval std::string_view source_header() { return "components/stt_whisper/stt_whisper.hpp"; }

    struct inputs {
        port<"audio_in", AudioBuffer> audio_in;
        slider<"record", "", float, fp(0.f), fp(1.f), fp(0.f)> record; // hold
        slider<"send",   "", float, fp(0.f), fp(1.f), fp(0.f)> send;   // edge
        slider<"erase",  "", float, fp(0.f), fp(1.f), fp(0.f)> erase;  // edge
        ::text<"model"> model;  // empty → assets/models/ggml-tiny.en.bin
    } inputs;

    struct outputs {
        port<"bip",       float> bip;        // 1 on take start, 0.5 on stop
        port<"recording", float> recording;
        port<"busy",      float> busy;       // inference in flight
        port<"text", std::string> text;
        bang<"heard">             heard;
    } outputs;

    SttWhisperNode() = default;
    ~SttWhisperNode();
    SttWhisperNode(const SttWhisperNode&) = delete;
    SttWhisperNode& operator=(const SttWhisperNode&) = delete;

    void operator()(double time_s);

private:
    struct Shared {                      // worker → tick handoff
        std::mutex  m;
        std::string text;
        bool        fresh = false;
        bool        busy  = false;
        whisper_context* ctx = nullptr;  // lazy-loaded, warm thereafter
    };
    std::shared_ptr<Shared> sh_ = std::make_shared<Shared>();
    std::vector<float> take_;            // 16 kHz mono accumulation
    bool  recording_  = false;
    bool  prev_send_  = false;
    bool  prev_erase_ = false;
    float ds_acc_     = 0.f;             // 48k → 16k decimator state
    int   ds_phase_   = 0;
};
