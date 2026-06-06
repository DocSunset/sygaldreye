// Copyright 2025 Travis West
#include "signal_graph.hpp"
#include "component_registry.hpp"
#include "eyeballs_node_abi.h"
#include <gtest/gtest.h>

static int nodeA_process_count = 0;
static int nodeB_process_count = 0;

static EyeballsNodeDescriptor make_node_a_desc() {
    return {
        .version       = EYEBALLS_ABI_VERSION,
        .type_name     = "node_a",
        .description   = nullptr,
        .create        = []() -> void* { return new int{0}; },
        .destroy       = [](void* p) { delete static_cast<int*>(p); },
        .process       = [](void*, double) { ++nodeA_process_count; },
        .serialize     = [](void*) -> const char* { return strdup("{}"); },
        .free_str      = [](const char* s) { free(const_cast<char*>(s)); },
        .deserialize   = [](void*, const char*) {},
        .push_textures = nullptr,
    };
}

static EyeballsNodeDescriptor make_node_b_desc() {
    return {
        .version       = EYEBALLS_ABI_VERSION,
        .type_name     = "node_b",
        .description   = nullptr,
        .create        = []() -> void* { return new int{0}; },
        .destroy       = [](void* p) { delete static_cast<int*>(p); },
        .process       = [](void*, double) { ++nodeB_process_count; },
        .serialize     = [](void*) -> const char* { return strdup("{}"); },
        .free_str      = [](const char* s) { free(const_cast<char*>(s)); },
        .deserialize   = [](void*, const char*) {},
        .push_textures = nullptr,
    };
}

TEST(SignalGraph, ParseAndTick) {
    static auto desc_a = make_node_a_desc();
    static auto desc_b = make_node_b_desc();

    ComponentRegistry reg;
    reg.register_builtin(&desc_a);
    reg.register_builtin(&desc_b);

    const char* json = R"({"nodes":[{"id":"a","type":"node_a","params":{}},{"id":"b","type":"node_b","params":{}}],"edges":[]})";

    nodeA_process_count = 0;
    nodeB_process_count = 0;

    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);
    ASSERT_EQ(g->nodes.size(), 2u);

    tick_graph(*g, 0.0);
    EXPECT_EQ(nodeA_process_count, 1);
    EXPECT_EQ(nodeB_process_count, 1);
}

TEST(SignalGraph, UnknownTypeReturnsNull) {
    ComponentRegistry reg;
    const char* json = R"({"nodes":[{"id":"x","type":"unknown"}],"edges":[]})";
    auto g = parse_graph(json, reg);
    EXPECT_EQ(g, nullptr);
}

TEST(SignalGraph, SerializeRoundTrip) {
    static auto desc_a = make_node_a_desc();
    ComponentRegistry reg;
    reg.register_builtin(&desc_a);

    const char* json = R"({"nodes":[{"id":"mynode","type":"node_a"}],"edges":[]})";
    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);

    auto s = serialize_graph(*g);
    EXPECT_NE(s.find("mynode"), std::string::npos);
    EXPECT_NE(s.find("node_a"), std::string::npos);
}
