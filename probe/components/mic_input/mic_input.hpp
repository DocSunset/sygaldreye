// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string_view>
#include <vector>

class MicInputNode {
public:
    static consteval std::string_view name() { return "mic_input"; }
    static constexpr int lift_kind() { return lift::resource_holder; }

    struct endpoints {
        normalled_in<float, fp(0.f), fp(10.f), fp(1.f)> gain;
        out<AudioBuffer> audio_out;
    } endpoints;

    MicInputNode();
    ~MicInputNode();
    MicInputNode(const MicInputNode&) = delete;
    MicInputNode& operator=(const MicInputNode&) = delete;

    void operator()(double time_s);

private:
    std::vector<float> tick_buf_;
};
