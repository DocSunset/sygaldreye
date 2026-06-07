// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <string_view>

class TriggerEdge {
public:
    static consteval std::string_view name()          { return "trigger_edge"; }
    static consteval std::string_view source_header() { return "components/trigger_edge/trigger_edge.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/trigger_edge/trigger_edge.cpp"; }

    struct inputs {
        port<"trigger", float>                                              trigger;
        slider<"threshold", "", float, fp(0.f), fp(1.f), fp(0.5f)>        threshold;
    } inputs;

    struct outputs {
        port<"press",   float> press;
        port<"release", float> release;
        port<"held",    float> held;
    } outputs;

    void operator()(double);

private:
    float prev_ = 0.f;
};
