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

    struct endpoints {
        normalled_in<float, fp(10.f), fp(3600.f), fp(180.f)> day_period_s;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.25f)> phase_offset;
        normalled_in<float, fp(0.f), fp(1.f), fp(0.05f)> min_intensity;
        normalled_in<float, fp(0.f), fp(5.f), fp(1.3f)> max_intensity;
    
        ::out<Eigen::Vector3f> direction;
        ::out<Eigen::Vector3f> color;
        ::out<float> intensity;
        ::out<float> elevation_norm;
    } endpoints;


    void operator()(double time_s);
};
