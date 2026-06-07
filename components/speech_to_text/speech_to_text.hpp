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
        port<"begin",     float>       begin;
        port<"finalize",  float>       finalize;
    } inputs;

    struct outputs {} outputs;

    std::string companion_url = "http://192.168.1.1:9090";

    void operator()(double time_s);

private:
    PushToTalk ptt_;
    bool       recording_ = false;
};

std::string to_json(const SpeechToTextNode& node);
void from_json(SpeechToTextNode& node, std::string_view json);
