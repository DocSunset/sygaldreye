// Copyright 2025 Travis West
#include "eyeballs_node_abi.hpp"
#include "sygaldry_endpoints.hpp"
#include <gtest/gtest.h>
#include <cstring>
#include <string>

struct TestNode {
    static consteval std::string_view name() { return "test_node"; }
    struct inputs {
        slider<"gain", "", float, 0.0f, 1.0f, 0.5f> gain;
    } inputs;
    void operator()(double) {}
};

struct NoProcessNode {
    static consteval std::string_view name() { return "no_process"; }
    struct inputs {
        toggle<"active"> active;
    } inputs;
};

TEST(EyeballsNodeAbi, Version) {
    EXPECT_EQ(make_descriptor<TestNode>()->version, EYEBALLS_ABI_VERSION);
}

TEST(EyeballsNodeAbi, TypeName) {
    EXPECT_STREQ(make_descriptor<TestNode>()->type_name, "test_node");
}

TEST(EyeballsNodeAbi, ProcessNotNull) {
    EXPECT_NE(make_descriptor<TestNode>()->process, nullptr);
}

TEST(EyeballsNodeAbi, NoProcessIsNull) {
    EXPECT_EQ(make_descriptor<NoProcessNode>()->process, nullptr);
}

TEST(EyeballsNodeAbi, CreateDestroy) {
    auto* d = make_descriptor<TestNode>();
    void* node = d->create();
    ASSERT_NE(node, nullptr);
    d->destroy(node);
}

TEST(EyeballsNodeAbi, SerializeContainsGain) {
    auto* d = make_descriptor<TestNode>();
    void* node = d->create();
    const char* json = d->serialize(node);
    ASSERT_NE(json, nullptr);
    EXPECT_NE(std::string(json).find("gain"), std::string::npos);
    d->free_str(json);
    d->destroy(node);
}

TEST(EyeballsNodeAbi, DeserializeRoundtrip) {
    auto* d = make_descriptor<TestNode>();
    void* node = d->create();
    d->deserialize(node, R"({"gain":0.75})");
    const char* json = d->serialize(node);
    EXPECT_NE(std::string(json).find("0.75"), std::string::npos);
    d->free_str(json);
    d->destroy(node);
}
