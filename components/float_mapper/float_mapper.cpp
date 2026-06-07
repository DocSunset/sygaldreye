// Copyright 2025 Travis West
#include "float_mapper.hpp"
#include <algorithm>
#include <cmath>

void FloatMapper::operator()(double) {
    float in      = inputs.in.value;
    float in_min  = inputs.in_min.value;
    float in_max  = inputs.in_max.value;
    float out_min = inputs.out_min.value;
    float out_max = inputs.out_max.value;
    float curve   = inputs.curve.value;

    float range = in_max - in_min;
    float t = (range != 0.f) ? (in - in_min) / range : 0.f;
    t = std::clamp(t, 0.f, 1.f);
    t = std::pow(t, curve);
    outputs.out.value = out_min + t * (out_max - out_min);
}
