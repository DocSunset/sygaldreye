// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string_view>

class TriggerEdge {
public:
    static consteval std::string_view name()          { return "trigger_edge"; }
    static consteval std::string_view source_header() { return "components/trigger_edge/trigger_edge.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/trigger_edge/trigger_edge.cpp"; }

    struct endpoints {
        in<float> trigger;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> threshold;
        out<float> press, release, held;
    } endpoints;

    void operator()(double);

private:
    float prev_ = 0.f;
};
