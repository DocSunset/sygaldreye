// Copyright 2025 Travis West
#include "eyeballs_node_abi.hpp"

#include <gtest/gtest.h>

#include <cstring>
#include <Eigen/Core>
#include <string>

#include "sygaldry_endpoints.hpp"

struct TestNode {
    static consteval std::string_view name() { return "test_node"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(1.f), fp(0.5f)> gain;
    } endpoints;
    void operator()(double) {}
};

struct NoProcessNode {
    static consteval std::string_view name() { return "no_process"; }
    struct endpoints {
        normalled_in<bool> active;
    } endpoints;
};

struct RichNode {
    static consteval std::string_view name() { return "rich_node"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(10.f), fp(1.f)> speed;
        in<Eigen::Vector3f> dir;
        out<Eigen::Vector3f> pos;
        out<MeshPtr> render;
    } endpoints;
    void operator()(double) {}
};

// v7: a node that opts out of the stateful lift default + names a key field.
struct LiftMetaNode {
    static consteval std::string_view name() { return "lift_meta"; }
    static constexpr int lift_kind() { return EYEBALLS_LIFT_STATELESS; }
    static constexpr std::string_view lift_key() { return "id"; }
    struct endpoints {
        normalled_in<std::string> id;
        out<float> y;
    } endpoints;
    void operator()(double) {}
};

TEST(EyeballsNodeAbi, Version) {
    EXPECT_EQ(make_descriptor<TestNode>()->version, EYEBALLS_ABI_VERSION);
}

TEST(EyeballsNodeAbi, LiftMetadataDefaultsStateful) {
    auto* d = make_descriptor<TestNode>();
    EXPECT_EQ(d->lift_kind, EYEBALLS_LIFT_STATEFUL);
    EXPECT_EQ(d->lift_key, nullptr);
}

TEST(EyeballsNodeAbi, LiftMetadataFromStaticMembers) {
    auto* d = make_descriptor<LiftMetaNode>();
    EXPECT_EQ(d->lift_kind, EYEBALLS_LIFT_STATELESS);
    ASSERT_NE(d->lift_key, nullptr);
    EXPECT_STREQ(d->lift_key, "id");
}

TEST(EyeballsNodeAbi, TypeName) {
    EXPECT_STREQ(make_descriptor<TestNode>()->type_name, "test_node");
}

TEST(EyeballsNodeAbi, ProcessNotNull) { EXPECT_NE(make_descriptor<TestNode>()->process, nullptr); }

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

TEST(EyeballsNodeAbi, DescriptorCarriesCurrentAbiVersion) {
    auto* d = make_descriptor<RichNode>();
    EXPECT_EQ(d->version, EYEBALLS_ABI_VERSION);
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
    rich->endpoints.pos.value = Eigen::Vector3f{1.0f, 2.0f, 3.0f};

    EyeballsOutputCtx ctx{};
    ctx.store = nullptr;
    ctx.node_id = "rich_node_0";
    ctx.emit_scalar = [](void*, const char*, const char*, double) {};
    ctx.emit_vec2 = [](void*, const char*, const char*, float, float) {};
    ctx.emit_vec3 = [](void* store, const char*, const char*, float x, float y, float z) {
        auto* arr = static_cast<float*>(store);
        arr[0] = x;
        arr[1] = y;
        arr[2] = z;
    };
    ctx.emit_vec4 = [](void*, const char*, const char*, float, float, float, float) {};
    ctx.emit_mat4 = [](void*, const char*, const char*, const float*) {};
    ctx.emit_quat = [](void*, const char*, const char*, float, float, float, float) {};
    ctx.emit_texture =
        [](void*, const char*, const char*, unsigned int, int, int, unsigned int, unsigned int) {};
    ctx.emit_audio = [](void*, const char*, const char*, const float*, int, int, int) {};

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

// ── endpoints v6 (kanban/backlog/endpoints_v6.md gate tests) ─────────────────

struct V6Producer {
    static consteval std::string_view name() { return "v6_producer"; }
    struct endpoints {
        out<float> level;
        out<AudioBuffer> audio;
        event_out tick;
    } endpoints;
    void operator()(double) {
        endpoints.level.value = 42.f;
        endpoints.tick.triggered = true;
    }
};

struct V6Consumer {
    static consteval std::string_view name() { return "v6_consumer"; }
    struct endpoints {
        in<float, fp(2.f)> gain;                               // compile-time default
        normalled_in<float, fp(0.f), fp(10.f), fp(5.f)> base;  // persisted param
        cv_in<float> depth;                                    // attenuverter
        normalled_in<std::string> label;
        in<AudioBuffer> audio;
        event_in trigger;
        out<float> sum;
    } endpoints;
    void operator()(double) {
        endpoints.sum.value = endpoints.gain.get() + endpoints.base.get() + endpoints.depth.get();
    }
};

TEST(EndpointsV6, TotalGetReadsDefaultsUnwired) {
    V6Consumer n;
    EXPECT_FLOAT_EQ(n.endpoints.gain.get(), 2.f);      // in<T,Def>
    EXPECT_FLOAT_EQ(n.endpoints.base.get(), 5.f);      // normalled init
    EXPECT_FLOAT_EQ(n.endpoints.depth.get(), 0.f);     // cv offset
    EXPECT_EQ(n.endpoints.audio.get().data, nullptr);  // stream absent
}

TEST(EndpointsV6, ConnectIsZeroCopyPointerWiring) {
    auto* dp = make_descriptor<V6Producer>();
    auto* dc = make_descriptor<V6Consumer>();
    ASSERT_NE(dp->connect, nullptr);
    ASSERT_NE(dp->output_ptr, nullptr);
    void* prod = dp->create();
    void* cons = dc->create();

    const void* src = dp->output_ptr(prod, "level");
    ASSERT_NE(src, nullptr);
    EXPECT_EQ(dc->connect(cons, "gain", src), 1);
    EXPECT_EQ(dc->connect(cons, "no_such_port", src), 0);

    dp->process(prod, 0.0);
    auto* c = static_cast<V6Consumer*>(cons);
    // the consumer reads the producer's storage DIRECTLY — no copies
    EXPECT_EQ(static_cast<const void*>(c->endpoints.gain.src), src);
    EXPECT_FLOAT_EQ(c->endpoints.gain.get(), 42.f);

    // disconnect: falls back to the compile-time default
    EXPECT_EQ(dc->connect(cons, "gain", nullptr), 1);
    EXPECT_FLOAT_EQ(c->endpoints.gain.get(), 2.f);

    dp->destroy(prod);
    dc->destroy(cons);
}

TEST(EndpointsV6, CvApplyAffineAndQuat) {
    V6Consumer n;
    float mod = 3.f;
    n.endpoints.depth.src = &mod;
    n.endpoints.depth.offset = 1.f;
    n.endpoints.depth.slope = -2.f;
    EXPECT_FLOAT_EQ(n.endpoints.depth.get(), 1.f - 2.f * 3.f);

    Eigen::Quaternionf base{Eigen::AngleAxisf(0.5f, Eigen::Vector3f::UnitY())};
    Eigen::Quaternionf src{Eigen::AngleAxisf(1.0f, Eigen::Vector3f::UnitX())};
    // slope 0 → offset; slope 1 → offset * src
    EXPECT_TRUE(cv_apply(src, 0.f, base).isApprox(base));
    EXPECT_TRUE(cv_apply(src, 1.f, base).isApprox(base * src));
}

TEST(EndpointsV6, SerializeEmitsPersistedFieldsNotLiveValues) {
    auto* dc = make_descriptor<V6Consumer>();
    void* cons = dc->create();
    auto* c = static_cast<V6Consumer*>(cons);

    float live = 9.f;
    c->endpoints.base.src = &live;     // wired: live value is 9
    c->endpoints.base.fallback = 5.f;  // persisted default stays 5
    c->endpoints.depth.offset = 0.25f;
    c->endpoints.depth.slope = -1.f;
    c->endpoints.label.fallback = "hi\nthere";

    const char* s = dc->serialize(cons);
    std::string j{s};
    dc->free_str(s);
    EXPECT_NE(j.find("\"base\":5"), std::string::npos);  // not 9
    EXPECT_NE(j.find("\"depth\":0.25"), std::string::npos);
    EXPECT_NE(j.find("\"depth_slope\":-1"), std::string::npos);
    EXPECT_NE(j.find("\"label\":\"hi\\nthere\""), std::string::npos);
    EXPECT_EQ(j.find("\"gain\""), std::string::npos);  // in<T>: no param

    // round-trip into a fresh instance
    void* cons2 = dc->create();
    dc->deserialize(cons2, j.c_str());
    auto* c2 = static_cast<V6Consumer*>(cons2);
    EXPECT_FLOAT_EQ(c2->endpoints.base.fallback, 5.f);
    EXPECT_FLOAT_EQ(c2->endpoints.depth.offset, 0.25f);
    EXPECT_FLOAT_EQ(c2->endpoints.depth.slope, -1.f);
    EXPECT_EQ(c2->endpoints.label.fallback, "hi\nthere");

    dc->destroy(cons);
    dc->destroy(cons2);
}

TEST(EndpointsV6, SchemaDirectionFromShapes) {
    auto* dc = make_descriptor<V6Consumer>();
    std::string s{dc->port_schema};
    auto inputs_at = s.find("\"inputs\"");
    auto outputs_at = s.find("\"outputs\"");
    ASSERT_NE(inputs_at, std::string::npos);
    ASSERT_NE(outputs_at, std::string::npos);
    EXPECT_LT(s.find("\"gain\""), outputs_at);  // inputs before outputs
    EXPECT_GT(s.find("\"sum\""), outputs_at);
    EXPECT_NE(s.find("\"min\":0,\"max\":10"), std::string::npos);  // normalled range
    EXPECT_NE(s.find("\"trigger\",\"kind\":\"bang\""), std::string::npos);
}

TEST(EndpointsV6, ParamWritersTargetPersistedFields) {
    auto* dc = make_descriptor<V6Consumer>();
    void* cons = dc->create();
    auto* c = static_cast<V6Consumer*>(cons);
    dc->set_scalar_in(cons, "base", 7.5);
    EXPECT_FLOAT_EQ(c->endpoints.base.fallback, 7.5f);
    dc->set_scalar_in(cons, "depth", 0.5);
    EXPECT_FLOAT_EQ(c->endpoints.depth.offset, 0.5f);
    dc->set_scalar_in(cons, "trigger", 1.0);
    EXPECT_TRUE(c->endpoints.trigger.triggered);
    dc->destroy(cons);
}
