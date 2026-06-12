// Copyright 2025 Travis West
#include "param_registry.hpp"
#include "sygaldry_endpoints.hpp"
#include <gtest/gtest.h>
#include <string>
#include <string_view>

struct TestNode {
    struct endpoints {
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> gain;
        normalled_in<bool>                              mute;
    } endpoints;
};

TEST(ParamRegistry, ToJsonContainsGain) {
    TestNode node;
    std::string json = to_json(node);
    EXPECT_NE(json.find("\"gain\""), std::string::npos);
}

TEST(ParamRegistry, ToJsonContainsMute) {
    TestNode node;
    std::string json = to_json(node);
    EXPECT_NE(json.find("\"mute\""), std::string::npos);
}

TEST(ParamRegistry, ToJsonDefaultValues) {
    TestNode node;
    std::string json = to_json(node);
    EXPECT_NE(json.find("0.5"), std::string::npos);
    EXPECT_NE(json.find("false"), std::string::npos);
}

TEST(ParamRegistry, FromJsonSetsGain) {
    TestNode node;
    from_json(node, R"({"gain":0.8})");
    EXPECT_NEAR(node.endpoints.gain.fallback, 0.8f, 1e-5f);
}

TEST(ParamRegistry, FromJsonIgnoresUnknownKeys) {
    TestNode node;
    from_json(node, R"({"unknown":42,"gain":0.3})");
    EXPECT_NEAR(node.endpoints.gain.fallback, 0.3f, 1e-5f);
}

TEST(ParamRegistry, FromJsonSetsMute) {
    TestNode node;
    from_json(node, R"({"mute":true})");
    EXPECT_TRUE(node.endpoints.mute.fallback);
}
