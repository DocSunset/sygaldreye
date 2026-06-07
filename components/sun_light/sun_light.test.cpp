// Copyright 2025 Travis West
#include "sun_light.hpp"
#include <gtest/gtest.h>

static SunLight make(double time_s,
                     float period = 180.f, float offset = 0.25f,
                     float min_int = 0.05f, float max_int = 1.3f) {
    SunLight n;
    n.inputs.day_period_s.value  = period;
    n.inputs.phase_offset.value  = offset;
    n.inputs.min_intensity.value = min_int;
    n.inputs.max_intensity.value = max_int;
    n(time_s);
    return n;
}

TEST(SunLight, Midday) {
    // phase_offset=0.25 means t=0 is midday
    auto n = make(0.0);
    EXPECT_NEAR(n.outputs.elevation_norm.value,  1.0f, 1e-5f);
    EXPECT_NEAR(n.outputs.intensity.value,        1.3f, 1e-4f);
    EXPECT_NEAR(n.outputs.direction.value.y(),   -1.0f, 1e-5f);
    EXPECT_NEAR(n.outputs.color.value.x(),        1.0f, 1e-5f);
}

TEST(SunLight, Midnight) {
    // At phase=0.75 (t = 0.5 * period with offset=0.25), elev_norm = -1
    auto n = make(90.0); // 90 s = 0.5 period
    EXPECT_NEAR(n.outputs.elevation_norm.value, -1.0f, 1e-5f);
    EXPECT_FLOAT_EQ(n.outputs.intensity.value,   0.05f);
}

TEST(SunLight, ColorWarmAtMidday) {
    auto n = make(0.0);
    // warm ≈ 0 at midday (|elev_norm|=1 → 1-3.5 < 0), so color is full
    EXPECT_FLOAT_EQ(n.outputs.color.value.x(), 1.0f);
    EXPECT_FLOAT_EQ(n.outputs.color.value.y(), 1.0f);
    EXPECT_FLOAT_EQ(n.outputs.color.value.z(), 1.0f);
}
