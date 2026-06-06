// Copyright 2025 Travis West
#include "sygaldry_endpoints.hpp"
#include <gtest/gtest.h>
#include <Eigen/Core>
#include <string_view>

using std::string_view;

TEST(SygaldryEndpoints, SliderName) {
    using S = slider<"wave height", "", float, 0.f, 20.f, 5.f>;
    EXPECT_EQ(S::name(), string_view("wave height"));
}

TEST(SygaldryEndpoints, SliderMinMaxInit) {
    using S = slider<"gain", "", float, 0.f, 1.f, 0.5f>;
    EXPECT_EQ(S::min(),  0.f);
    EXPECT_EQ(S::max(),  1.f);
    EXPECT_EQ(S::init(), 0.5f);
}

TEST(SygaldryEndpoints, SliderValueInitializesToInit) {
    slider<"gain", "", float, 0.f, 1.f, 0.5f> s;
    EXPECT_EQ(s.value, 0.5f);
}

TEST(SygaldryEndpoints, ToggleName) {
    using T = toggle<"mute">;
    EXPECT_EQ(T::name(), string_view("mute"));
}

TEST(SygaldryEndpoints, ToggleDefaultFalse) {
    toggle<"mute"> t;
    EXPECT_FALSE(t.value);
}

TEST(SygaldryEndpoints, BangName) {
    using B = bang<"fire">;
    EXPECT_EQ(B::name(), string_view("fire"));
}

TEST(SygaldryEndpoints, BangDefaultFalse) {
    bang<"fire"> b;
    EXPECT_FALSE(b.triggered);
}

TEST(SygaldryEndpoints, PortName) {
    using P = port<"pos", Eigen::Vector3f>;
    EXPECT_EQ(P::name(), string_view("pos"));
}

TEST(SygaldryEndpoints, PortValueType) {
    port<"pos", Eigen::Vector3f> p;
    static_assert(std::is_same_v<decltype(p)::value_type, Eigen::Vector3f>);
    EXPECT_EQ(p.value.norm(), 0.0f);
}

TEST(SygaldryEndpoints, PortAudioBuffer) {
    using P = port<"audio", AudioBuffer>;
    EXPECT_EQ(P::name(), string_view("audio"));
    P p;
    EXPECT_EQ(p.value.data, nullptr);
    EXPECT_EQ(p.value.frames, 0);
}

TEST(SygaldryEndpoints, PortDrawFn) {
    using P = port<"render", DrawFn>;
    EXPECT_EQ(P::name(), string_view("render"));
    P p;
    EXPECT_FALSE(static_cast<bool>(p.value));
}
