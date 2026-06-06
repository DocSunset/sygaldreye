// Copyright 2025 Travis West
#include "eyeballs_node_abi.hpp"
#include "sygaldry_endpoints.hpp"
#include <Eigen/Core>
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

struct RichNode {
    static consteval std::string_view name() { return "rich_node"; }
    struct inputs {
        slider<"speed", "", float, 0.0f, 10.0f, 1.0f> speed;
        port<"dir", Eigen::Vector3f> dir;
    } inputs;
    struct outputs {
        port<"pos", Eigen::Vector3f> pos;
        port<"render", DrawFn>       render;
    } outputs;
    void operator()(double) {}
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

TEST(EyeballsNodeAbi, AbiVersionIsV4) {
    EXPECT_EQ(EYEBALLS_ABI_VERSION, 4);
}

TEST(EyeballsNodeAbi, PushOutputsNotNull) {
    auto* d = make_descriptor<RichNode>();
    EXPECT_NE(d->push_outputs, nullptr);
}

TEST(EyeballsNodeAbi, SetScalarInNotNull) {
    auto* d = make_descriptor<RichNode>();
    EXPECT_NE(d->set_scalar_in, nullptr);
}

TEST(EyeballsNodeAbi, SetVec3InNotNull) {
    auto* d = make_descriptor<RichNode>();
    EXPECT_NE(d->set_vec3_in, nullptr);
}

TEST(EyeballsNodeAbi, SetScalarInSetsValue) {
    auto* d = make_descriptor<RichNode>();
    void* node = d->create();
    d->set_scalar_in(node, "speed", 7.0);
    // Verify by serializing and checking value
    const char* json = d->serialize(node);
    EXPECT_NE(std::string(json).find("7"), std::string::npos);
    d->free_str(json);
    d->destroy(node);
}

TEST(EyeballsNodeAbi, PushOutputsCallsEmitVec3) {
    auto* d = make_descriptor<RichNode>();
    void* node = d->create();
    // Set the pos output value via direct cast
    auto* rich = static_cast<RichNode*>(node);
    rich->outputs.pos.value = Eigen::Vector3f{1.0f, 2.0f, 3.0f};

    EyeballsOutputCtx ctx{};
    ctx.store   = nullptr;
    ctx.node_id = "rich_node_0";
    ctx.emit_scalar  = [](void*, const char*, const char*, double) {};
    ctx.emit_vec2    = [](void*, const char*, const char*, float, float) {};
    ctx.emit_vec3    = [](void* store, const char*, const char*,
                          float x, float y, float z) {
        auto* arr = static_cast<float*>(store);
        arr[0] = x; arr[1] = y; arr[2] = z;
    };
    ctx.emit_vec4    = [](void*, const char*, const char*, float, float, float, float) {};
    ctx.emit_mat4    = [](void*, const char*, const char*, const float*) {};
    ctx.emit_quat    = [](void*, const char*, const char*, float, float, float, float) {};
    ctx.emit_texture = [](void*, const char*, const char*, unsigned int, int, int,
                          unsigned int, unsigned int) {};
    ctx.emit_audio   = [](void*, const char*, const char*, const float*, int, int, int) {};

    float out[3] = {0, 0, 0};
    ctx.store = out;

    d->push_outputs(node, &ctx);
    EXPECT_FLOAT_EQ(out[0], 1.0f);
    EXPECT_FLOAT_EQ(out[1], 2.0f);
    EXPECT_FLOAT_EQ(out[2], 3.0f);

    d->destroy(node);
}

TEST(EyeballsNodeAbi, PortSchemaNotNull) {
    auto* d = make_descriptor<RichNode>();
    EXPECT_NE(d->port_schema, nullptr);
    EXPECT_NE(std::string(d->port_schema).find("inputs"), std::string::npos);
    EXPECT_NE(std::string(d->port_schema).find("outputs"), std::string::npos);
}
