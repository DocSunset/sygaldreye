// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string_view>

class PttGate {
public:
    static consteval std::string_view name()          { return "ptt_gate"; }
    static consteval std::string_view source_header() { return "components/ptt_gate/ptt_gate.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/ptt_gate/ptt_gate.cpp"; }

    struct endpoints {
        in<AudioBuffer> audio_in;
        in<float>       open, close;
        out<AudioBuffer> audio_out;
        out<float>       opened, closed, is_open;
    } endpoints;

    void operator()(double time_s);

private:
    bool open_ = false;
};
