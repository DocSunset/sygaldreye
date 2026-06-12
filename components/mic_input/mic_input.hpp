// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string_view>
#include <vector>

class MicInputNode {
public:
    static consteval std::string_view name() { return "mic_input"; }

    struct inputs {
        slider<"gain", "", float, fp(0.f), fp(10.f), fp(1.f)> gain;
    } inputs;

    struct outputs {
        port<"audio_out", AudioBuffer> audio_out;
    } outputs;

    MicInputNode();
    ~MicInputNode();
    MicInputNode(const MicInputNode&) = delete;
    MicInputNode& operator=(const MicInputNode&) = delete;

    void operator()(double time_s);

private:
    std::vector<float> tick_buf_;
};
