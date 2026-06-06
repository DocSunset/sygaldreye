// Copyright 2025 Travis West
#include "sygaldry_endpoints.hpp"
#include <gtest/gtest.h>
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
