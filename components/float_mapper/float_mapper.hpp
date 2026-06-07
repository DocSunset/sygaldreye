// Copyright 2025 Travis West
#pragma once
#include "sygaldry_endpoints.hpp"
#include <cmath>
#include <string_view>

class FloatMapper {
public:
    static consteval std::string_view name()          { return "float_mapper"; }
    static consteval std::string_view source_header() { return "components/float_mapper/float_mapper.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/float_mapper/float_mapper.cpp"; }

    struct inputs {
        port<"in",  float>                                                  in;
        slider<"in_min",  "", float, fp(-1000.f), fp(1000.f), fp(0.f)>    in_min;
        slider<"in_max",  "", float, fp(-1000.f), fp(1000.f), fp(1.f)>    in_max;
        slider<"out_min", "", float, fp(-1000.f), fp(1000.f), fp(0.f)>    out_min;
        slider<"out_max", "", float, fp(-1000.f), fp(1000.f), fp(1.f)>    out_max;
        slider<"curve",   "", float, fp(0.1f),    fp(10.f),   fp(1.f)>    curve;
    } inputs;

    struct outputs {
        port<"out", float> out;
    } outputs;

    void operator()(double);
};
