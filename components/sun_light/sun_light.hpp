// Copyright 2025 Travis West
#pragma once
#include <Eigen/Core>
#include <string_view>
#include "sygaldry_endpoints.hpp"

class SunLight {
public:
    static consteval std::string_view name()          { return "sun_light"; }
    static consteval std::string_view source_header() { return "components/sun_light/sun_light.hpp"; }
    static consteval std::string_view source_cpp()    { return "components/sun_light/sun_light.cpp"; }

    struct inputs {
        slider<"day_period_s",  "", float, fp(10.f),  fp(3600.f), fp(180.f)> day_period_s;
        slider<"phase_offset",  "", float, fp(0.f),   fp(1.f),    fp(0.25f)> phase_offset;
        slider<"min_intensity", "", float, fp(0.f),   fp(1.f),    fp(0.05f)> min_intensity;
        slider<"max_intensity", "", float, fp(0.f),   fp(5.f),    fp(1.3f)>  max_intensity;
    } inputs;

    struct outputs {
        port<"direction",      Eigen::Vector3f> direction;
        port<"color",          Eigen::Vector3f> color;
        port<"intensity",      float>           intensity;
        port<"elevation_norm", float>           elevation_norm;
    } outputs;

    void operator()(double time_s);
};
