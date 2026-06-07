// Copyright 2025 Travis West
#include "sun_light.hpp"
#include <algorithm>
#include <cmath>

void SunLight::operator()(double time_s) {
    float period  = inputs.day_period_s.value;
    float offset  = inputs.phase_offset.value;
    float min_int = inputs.min_intensity.value;
    float max_int = inputs.max_intensity.value;

    float phase     = static_cast<float>(fmod(time_s / period + offset, 1.0));
    float elev_norm = sinf(2.0f * static_cast<float>(M_PI) * phase);
    float elev_rad  = elev_norm * (static_cast<float>(M_PI) / 2.0f);
    float azimuth   = static_cast<float>(M_PI) * phase;
    float ce        = cosf(elev_rad);

    outputs.elevation_norm.value = elev_norm;
    outputs.direction.value = {-ce * cosf(azimuth), -sinf(elev_rad), -ce * sinf(azimuth)};

    float warm = std::max(0.0f, 1.0f - fabsf(elev_norm) * 3.5f);
    warm *= warm;
    outputs.color.value = {1.0f, 0.85f + 0.15f * (1.0f - warm), 0.65f + 0.35f * (1.0f - warm)};

    outputs.intensity.value = std::max(min_int, sinf(elev_rad) * max_int);
}
