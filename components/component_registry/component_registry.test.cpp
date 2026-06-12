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
    struct endpoints {
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> val;
    } endpoints;
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

#ifdef HOT_TEST_V1_PATH
TEST(HotReload, ReplaceRegisteredTypeFromNewSo) {
    ComponentRegistry reg;
    ASSERT_TRUE(reg.load_plugin(HOT_TEST_V1_PATH));
    const auto* d1 = reg.find("hot_test");
    ASSERT_NE(d1, nullptr);

    // A live instance of v1, as a running graph would hold.
    void* old_inst = d1->create();
    ASSERT_NE(old_inst, nullptr);
    d1->process(old_inst, 0.0);

    // Reload: same type name, new .so.
    ASSERT_TRUE(reg.load_plugin(HOT_TEST_V2_PATH));
    const auto* d2 = reg.find("hot_test");
    ASSERT_NE(d2, nullptr);
    EXPECT_NE(d1, d2);  // new descriptor identity → migration re-creates

    // Old code must stay executable until instances are gone (handle
    // retired, not dlclosed).
    d1->process(old_inst, 0.0);
    d1->destroy(old_inst);

    EXPECT_EQ(reg.type_names().size(), 1u);  // one name, newest wins
    // (Behavior change across versions is proven in
    // ParamsCarryAcrossDescriptorChange via the ticked output.)
}

TEST(HotReload, ParamsCarryAcrossDescriptorChange) {
    // The migration semantics a reload relies on: same node id, DIFFERENT
    // descriptor → no adoption; the fresh instance keeps params parsed from
    // the serialized live graph.
    ComponentRegistry reg1, reg2;
    ASSERT_TRUE(reg1.load_plugin(HOT_TEST_V1_PATH));
    ASSERT_TRUE(reg2.load_plugin(HOT_TEST_V2_PATH));

    auto g1 = parse_graph(
        R"({"nodes":[{"id":"h","type":"hot_test","params":{"gain":7}}],"edges":[]})",
        reg1);
    ASSERT_TRUE(g1);
    tick_graph(*g1, 0.0);
    EXPECT_DOUBLE_EQ(std::get<double>(g1->values.at("h.v")), 7.0);  // v1: ×1

    // What POST /plugins does: serialize the live graph, re-parse through
    // the new registry, migrate.
    auto g2 = parse_graph(serialize_graph(*g1), reg2);
    ASSERT_TRUE(g2);
    migrate_graph(*g2, *g1);
    tick_graph(*g2, 0.0);
    EXPECT_DOUBLE_EQ(std::get<double>(g2->values.at("h.v")), 14.0);  // v2: ×2, gain carried
}
#endif

TEST(ComponentRegistry, LoadJsonMalformed) {
    const char* path = "/tmp/eyeballs_bad.json";
    std::FILE* f = std::fopen(path, "wb");
    ASSERT_NE(f, nullptr);
    std::fputs("{invalid}", f);
    std::fclose(f);

    ComponentRegistry reg;
    EXPECT_FALSE(reg.load_plugin(path));
}
