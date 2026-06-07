// Copyright 2025 Travis West
#include "signal_graph.hpp"
#include "component_registry.hpp"
#include "eyeballs_node_abi.h"
#include "eyeballs_node_abi.hpp"
#include "sygaldry_endpoints.hpp"
#include <gtest/gtest.h>

static int nodeA_process_count = 0;
static int nodeB_process_count = 0;

static EyeballsNodeDescriptor make_node_a_desc() {
    return {
        .version          = EYEBALLS_ABI_VERSION,
        .type_name        = "node_a",
        .description      = nullptr,
        .create           = []() -> void* { return new int{0}; },
        .destroy          = [](void* p) { delete static_cast<int*>(p); },
        .process          = [](void*, double) { ++nodeA_process_count; },
        .serialize        = [](void*) -> const char* { return strdup("{}"); },
        .free_str         = [](const char* s) { free(const_cast<char*>(s)); },
        .deserialize      = [](void*, const char*) {},
        .push_textures    = nullptr,
        .source_header    = nullptr,
        .source_cpp       = nullptr,
        .port_schema      = nullptr,
        .push_outputs     = nullptr,
        .push_draw_calls  = nullptr,
        .set_scalar_in    = nullptr,
        .set_vec2_in      = nullptr,
        .set_vec3_in      = nullptr,
        .set_vec4_in      = nullptr,
        .set_mat4_in      = nullptr,
        .set_quat_in      = nullptr,
        .set_texture_in   = nullptr,
        .set_audio_in     = nullptr,
    };
}

static EyeballsNodeDescriptor make_node_b_desc() {
    return {
        .version          = EYEBALLS_ABI_VERSION,
        .type_name        = "node_b",
        .description      = nullptr,
        .create           = []() -> void* { return new int{0}; },
        .destroy          = [](void* p) { delete static_cast<int*>(p); },
        .process          = [](void*, double) { ++nodeB_process_count; },
        .serialize        = [](void*) -> const char* { return strdup("{}"); },
        .free_str         = [](const char* s) { free(const_cast<char*>(s)); },
        .deserialize      = [](void*, const char*) {},
        .push_textures    = nullptr,
        .source_header    = nullptr,
        .source_cpp       = nullptr,
        .port_schema      = nullptr,
        .push_outputs     = nullptr,
        .push_draw_calls  = nullptr,
        .set_scalar_in    = nullptr,
        .set_vec2_in      = nullptr,
        .set_vec3_in      = nullptr,
        .set_vec4_in      = nullptr,
        .set_mat4_in      = nullptr,
        .set_quat_in      = nullptr,
        .set_texture_in   = nullptr,
        .set_audio_in     = nullptr,
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

// ── Producer/consumer nodes for tick_graph propagation tests ──────────────────

// ProducerNode: has a float output port, emits a constant scalar value.
struct ProducerNode {
    static consteval std::string_view name() { return "producer"; }
    struct inputs {} inputs;
    struct outputs { port<"val", float> val; } outputs;
    void operator()(double) { outputs.val.value = 42.0f; }
};

// ConsumerNode: has a float input port, records the last value set.
struct ConsumerNode {
    static consteval std::string_view name() { return "consumer"; }
    struct inputs { port<"val", float> val; } inputs;
    struct outputs {} outputs;
    void operator()(double) {}
};

// DrawNode: has a DrawFn output port.
struct DrawNode {
    static consteval std::string_view name() { return "draw_node"; }
    static bool drawn;
    struct inputs {} inputs;
    struct outputs { port<"render", DrawFn> render; } outputs;
    void operator()(double) {
        outputs.render.value = [](const Eigen::Matrix4f&) { DrawNode::drawn = true; };
    }
};
bool DrawNode::drawn = false;

TEST(SignalGraph, TickGraphPropagatesTopoOrder) {
    // Producer (a) → Consumer (b): verify b's input gets a's output value
    static auto desc_a = make_descriptor<ProducerNode>();
    static auto desc_b = make_descriptor<ConsumerNode>();

    ComponentRegistry reg;
    reg.register_builtin(desc_a);
    reg.register_builtin(desc_b);

    const char* json = R"({
        "nodes":[
            {"id":"b","type":"consumer","params":{}},
            {"id":"a","type":"producer","params":{}}
        ],
        "edges":[
            {"from":"a.val","to":"b.val"}
        ]
    })";
    // Note: nodes added in reverse order (b before a) — topo sort must reorder.

    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);
    ASSERT_EQ(g->nodes.size(), 2u);
    ASSERT_EQ(g->edges.size(), 1u);

    tick_graph(*g, 0.0);

    // After tick: value store should have a.val = 42.0
    ASSERT_NE(g->values.find("a.val"), g->values.end());
    EXPECT_NEAR(std::get<double>(g->values["a.val"]), 42.0, 1e-6);

    // Consumer node's input should have been set to 42
    auto* consumer = static_cast<ConsumerNode*>(g->nodes[0].data);
    // node b is index 0 (as parsed)
    EXPECT_NEAR(consumer->inputs.val.value, 42.0f, 1e-5f);
}

TEST(SignalGraph, TickGraphCollectsDrawCalls) {
    static auto desc = make_descriptor<DrawNode>();
    ComponentRegistry reg;
    reg.register_builtin(desc);

    const char* json = R"({"nodes":[{"id":"d","type":"draw_node","params":{}}],"edges":[]})";
    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);

    DrawNode::drawn = false;
    tick_graph(*g, 0.0);
    ASSERT_EQ(g->draw_calls.size(), 1u);

    // Invoke the collected draw call
    g->draw_calls[0](Eigen::Matrix4f::Identity());
    EXPECT_TRUE(DrawNode::drawn);
}

TEST(SignalGraph, InletOutletRoundTrip) {
    static auto desc_a = make_node_a_desc();
    static auto desc_b = make_node_b_desc();
    ComponentRegistry reg;
    reg.register_builtin(&desc_a);
    reg.register_builtin(&desc_b);

    const char* json = R"({
        "nodes":[
            {"id":"a","type":"node_a","params":{}},
            {"id":"b","type":"node_b","params":{}}
        ],
        "edges":[],
        "inlets":[{"name":"in1","node":"a","port":"x"}],
        "outlets":[{"name":"out1","node":"b","port":"y"}]
    })";

    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);
    ASSERT_EQ(g->inlets.size(), 1u);
    EXPECT_EQ(g->inlets[0].name, "in1");
    EXPECT_EQ(g->inlets[0].node, "a");
    EXPECT_EQ(g->inlets[0].port, "x");
    ASSERT_EQ(g->outlets.size(), 1u);
    EXPECT_EQ(g->outlets[0].name, "out1");
    EXPECT_EQ(g->outlets[0].node, "b");
    EXPECT_EQ(g->outlets[0].port, "y");

    auto s = serialize_graph(*g);
    EXPECT_NE(s.find("\"inlets\""), std::string::npos);
    EXPECT_NE(s.find("\"outlets\""), std::string::npos);
    EXPECT_NE(s.find("in1"), std::string::npos);
    EXPECT_NE(s.find("out1"), std::string::npos);
}

TEST(SignalGraph, NoInletsOutletsIsEmpty) {
    static auto desc_a = make_node_a_desc();
    ComponentRegistry reg;
    reg.register_builtin(&desc_a);

    const char* json = R"({"nodes":[{"id":"a","type":"node_a","params":{}}],"edges":[]})";
    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);
    EXPECT_TRUE(g->inlets.empty());
    EXPECT_TRUE(g->outlets.empty());

    auto s = serialize_graph(*g);
    EXPECT_EQ(s.find("\"inlets\""), std::string::npos);
    EXPECT_EQ(s.find("\"outlets\""), std::string::npos);
}

TEST(SignalGraph, EdgeRoundTrip) {
    static auto desc_a = make_node_a_desc();
    static auto desc_b = make_node_b_desc();
    ComponentRegistry reg;
    reg.register_builtin(&desc_a);
    reg.register_builtin(&desc_b);

    const char* json = R"({
        "nodes":[
            {"id":"a","type":"node_a","params":{}},
            {"id":"b","type":"node_b","params":{}}
        ],
        "edges":[
            {"from":"a.out_val","to":"b.in_val"}
        ]
    })";

    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);
    ASSERT_EQ(g->edges.size(), 1u);
    EXPECT_EQ(g->edges[0].from_node, "a");
    EXPECT_EQ(g->edges[0].from_port, "out_val");
    EXPECT_EQ(g->edges[0].to_node,   "b");
    EXPECT_EQ(g->edges[0].to_port,   "in_val");

    // Serialize and verify edge appears in output
    auto s = serialize_graph(*g);
    EXPECT_NE(s.find("a.out_val"), std::string::npos);
    EXPECT_NE(s.find("b.in_val"),  std::string::npos);
}
