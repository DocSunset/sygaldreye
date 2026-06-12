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

    struct endpoints {
        in<AudioBuffer> audio_in;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> record; // hold
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> send;   // edge
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> erase;  // edge
        normalled_in<std::string> model;  // empty → assets/models/ggml-tiny.en.bin
        out<float> bip;        // 1 on take start, 0.5 on stop
        out<float> recording;
        out<float> busy;       // inference in flight
        out<std::string> text;
        event_out        heard;
    } endpoints;

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
