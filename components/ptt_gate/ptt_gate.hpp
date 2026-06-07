// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string_view>

class PttGate {
public:
    static consteval std::string_view name()          { return "ptt_gate"; }
    static consteval std::string_view source_header() { return "components/ptt_gate/ptt_gate.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/ptt_gate/ptt_gate.cpp"; }

    struct inputs {
        port<"audio_in", AudioBuffer> audio_in;
        port<"open",     float>       open;
        port<"close",    float>       close;
    } inputs;

    struct outputs {
        port<"audio_out", AudioBuffer> audio_out;
        port<"opened",    float>       opened;
        port<"closed",    float>       closed;
        port<"is_open",   float>       is_open;
    } outputs;

    void operator()(double time_s);

private:
    bool open_ = false;
};
