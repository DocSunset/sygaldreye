// Copyright 2025 Travis West
#pragma once
#include "push_to_talk.hpp"
#include "sygaldry_endpoints.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

class SpeechToTextNode {
public:
    static consteval std::string_view name()          { return "speech_to_text"; }
    static consteval std::string_view source_header() { return "components/speech_to_text/speech_to_text.hpp"; }
    static constexpr int lift_kind() { return lift::resource_holder; }
    static consteval std::string_view source_cpp()    { return "components/speech_to_text/speech_to_text.cpp"; }

    struct endpoints {
        in<AudioBuffer> audio_in;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> record; // hold
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> send;   // edge
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> erase;  // edge
        normalled_in<std::string> url;  // empty → http://192.168.1.1:9090/transcribe
        out<float> bip;        // 1 on take start, 0.5 on stop
        out<float> recording;
        // The transcript as a VALUE: wire stt.text → claude.message and
        // stt.heard → claude.send — no companion forwarding, no seq bump.
        out<std::string> text;
        event_out        heard;
    } endpoints;

    void operator()(double time_s);

private:
    struct Pending {
        std::mutex  m;
        std::string text;
        bool        fresh = false;
    };
    PushToTalk ptt_;
    std::shared_ptr<Pending> pending_ = std::make_shared<Pending>();
    bool recording_  = false;
    bool prev_send_  = false;
    bool prev_erase_ = false;
};
