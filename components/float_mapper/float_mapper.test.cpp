// Copyright 2025 Travis West
#include "float_mapper.hpp"
#include <gtest/gtest.h>

static FloatMapper make(float in, float in_min=0.f, float in_max=1.f,
                        float out_min=0.f, float out_max=1.f, float curve=1.f) {
    FloatMapper m;
    m.inputs.in.value      = in;
    m.inputs.in_min.value  = in_min;
    m.inputs.in_max.value  = in_max;
    m.inputs.out_min.value = out_min;
    m.inputs.out_max.value = out_max;
    m.inputs.curve.value   = curve;
    m(0.0);
    return m;
}

TEST(FloatMapper, LinearIdentity) {
    EXPECT_FLOAT_EQ(make(0.f).outputs.out.value, 0.f);
    EXPECT_FLOAT_EQ(make(1.f).outputs.out.value, 1.f);
    EXPECT_FLOAT_EQ(make(0.5f).outputs.out.value, 0.5f);
}

TEST(FloatMapper, RangeChange) {
    EXPECT_FLOAT_EQ(make(0.f, 0.f, 1.f, 10.f, 20.f).outputs.out.value, 10.f);
    EXPECT_FLOAT_EQ(make(1.f, 0.f, 1.f, 10.f, 20.f).outputs.out.value, 20.f);
    EXPECT_FLOAT_EQ(make(0.5f, 0.f, 1.f, 10.f, 20.f).outputs.out.value, 15.f);
}

TEST(FloatMapper, ClampBelowInMin) {
    EXPECT_FLOAT_EQ(make(-1.f, 0.f, 1.f, 5.f, 10.f).outputs.out.value, 5.f);
}

TEST(FloatMapper, ClampAboveInMax) {
    EXPECT_FLOAT_EQ(make(2.f, 0.f, 1.f, 5.f, 10.f).outputs.out.value, 10.f);
}

TEST(FloatMapper, QuadraticCurve) {
    float out = make(0.5f, 0.f, 1.f, 0.f, 1.f, 2.f).outputs.out.value;
    EXPECT_NEAR(out, 0.25f, 1e-5f);
}

TEST(FloatMapper, DegenerateRangeNoCrash) {
    FloatMapper m = make(0.5f, 1.f, 1.f, 3.f, 7.f);
    EXPECT_FLOAT_EQ(m.outputs.out.value, 3.f);
}
