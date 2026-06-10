// Copyright 2025 Travis West
#include "component_registry.hpp"
#include "subgraph_node.hpp"
#include "eyeballs_node_abi.hpp"
#include "sygaldry_endpoints.hpp"
#include <gtest/gtest.h>
#include <cstdio>
#include <string>

struct RegistryTestNode {
    static consteval std::string_view name() { return "registry_test"; }
    struct inputs {
        slider<"val", "", float, 0.0f, 1.0f, 0.5f> val;
    } inputs;
};

TEST(ComponentRegistry, RegisterBuiltinFind) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<RegistryTestNode>());
    const EyeballsNodeDescriptor* d = reg.find("registry_test");
    ASSERT_NE(d, nullptr);
    EXPECT_STREQ(d->type_name, "registry_test");
}

TEST(ComponentRegistry, FindMissingReturnsNull) {
    ComponentRegistry reg;
    EXPECT_EQ(reg.find("nonexistent"), nullptr);
}

TEST(ComponentRegistry, TypeNames) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<RegistryTestNode>());
    auto names = reg.type_names();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names[0], "registry_test");
}

TEST(ComponentRegistry, CreateDestroyViaRegistry) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<RegistryTestNode>());
    const EyeballsNodeDescriptor* d = reg.find("registry_test");
    ASSERT_NE(d, nullptr);
    void* node = d->create();
    ASSERT_NE(node, nullptr);
    d->destroy(node);
}

TEST(ComponentRegistry, LoadJsonSubgraph) {
    const char* path = "/tmp/test_sub.json";
    const char* json = "{\"nodes\":[],\"edges\":[]}";
    std::FILE* f = std::fopen(path, "wb");
    ASSERT_NE(f, nullptr);
    std::fputs(json, f);
    std::fclose(f);

    ComponentRegistry reg;
    EXPECT_TRUE(reg.load_plugin(path));
    EXPECT_NE(reg.find("test_sub"), nullptr);
}

TEST(ComponentRegistry, LoadJsonMalformed) {
    const char* path = "/tmp/eyeballs_bad.json";
    std::FILE* f = std::fopen(path, "wb");
    ASSERT_NE(f, nullptr);
    std::fputs("{invalid}", f);
    std::fclose(f);

    ComponentRegistry reg;
    EXPECT_FALSE(reg.load_plugin(path));
}
