// Copyright 2025 Travis West
#pragma once
#include "push_to_talk.hpp"
#include "sygaldry_endpoints.hpp"
#include <string>
#include <string_view>

class SpeechToTextNode {
public:
    static consteval std::string_view name()          { return "speech_to_text"; }
    static consteval std::string_view source_header() { return "components/speech_to_text/speech_to_text.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/speech_to_text/speech_to_text.cpp"; }

    struct inputs {
        port<"audio_in",  AudioBuffer> audio_in;
        slider<"record", "", float, fp(0.f), fp(1.f), fp(0.f)> record; // hold
        slider<"send",   "", float, fp(0.f), fp(1.f), fp(0.f)> send;   // edge
        slider<"erase",  "", float, fp(0.f), fp(1.f), fp(0.f)> erase;  // edge
        ::text<"url"> url;  // empty → http://192.168.1.1:9090/transcribe
    } inputs;

    struct outputs {
        port<"bip",       float> bip;        // 1 on take start, 0.5 on stop
        port<"recording", float> recording;
    } outputs;

    void operator()(double time_s);

private:
    PushToTalk ptt_;
    bool recording_  = false;
    bool prev_send_  = false;
    bool prev_erase_ = false;
};
