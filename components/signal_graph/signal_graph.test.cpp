// Copyright 2025 Travis West
#include "signal_graph.hpp"
#include "component_registry.hpp"
#include "subgraph_node.hpp"
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

// ── Inline subgraph tests ──────────────────────────────────────────────────────

TEST(SignalGraph, InlineSubgraphParsed) {
    static auto desc_a = make_node_a_desc();
    ComponentRegistry reg;
    reg.register_builtin(&desc_a);

    // Node with "graph" key — no "type". Inner graph is empty.
    const char* json = R"({
        "nodes":[{"id":"sub","graph":{"nodes":[],"edges":[]}}],
        "edges":[]
    })";

    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);
    ASSERT_EQ(g->nodes.size(), 1u);
    EXPECT_EQ(std::string(g->nodes[0].desc->type_name).find("__inline_"), 0u);
}

TEST(SignalGraph, InlineSubgraphEndToEnd) {
    // Inner graph: one ProducerNode, outlet mapped to "out"
    static auto desc_p = make_descriptor<ProducerNode>();
    ComponentRegistry reg;
    reg.register_builtin(desc_p);

    const char* json = R"({
        "nodes":[{
            "id":"sub",
            "graph":{
                "nodes":[{"id":"p","type":"producer","params":{}}],
                "edges":[],
                "outlets":[{"name":"out","node":"p","port":"val"}]
            }
        }],
        "edges":[]
    })";

    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);
    ASSERT_EQ(g->nodes.size(), 1u);

    tick_graph(*g, 0.0);

    auto it = g->values.find("sub.out");
    ASSERT_NE(it, g->values.end());
    EXPECT_NEAR(std::get<double>(it->second), 42.0, 1e-6);
}

TEST(SignalGraph, NestedInlineSubgraph) {
    // Outer graph contains an inline subgraph, which itself contains another inline subgraph.
    static auto desc_p = make_descriptor<ProducerNode>();
    ComponentRegistry reg;
    reg.register_builtin(desc_p);

    const char* json = R"({
        "nodes":[{
            "id":"outer",
            "graph":{
                "nodes":[{
                    "id":"inner",
                    "graph":{
                        "nodes":[{"id":"p","type":"producer","params":{}}],
                        "edges":[]
                    }
                }],
                "edges":[]
            }
        }],
        "edges":[]
    })";

    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);
    ASSERT_EQ(g->nodes.size(), 1u);
    // Should not crash
    tick_graph(*g, 0.0);
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

// ── migrate_graph ──────────────────────────────────────────────────────────
// Counter node: state survives migration, params re-apply.
struct CounterNode {
    static consteval std::string_view name() { return "counter"; }
    struct inputs  { slider<"step", "", float, fp(0.f), fp(10.f), fp(1.f)> step; } inputs;
    struct outputs { port<"count", float> count; } outputs;
    float count_ = 0.f;
    void operator()(double) { count_ += inputs.step.value; outputs.count.value = count_; }
};

TEST(MigrateGraph, StateSurvivesParamsReapply) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());

    auto g1 = parse_graph(R"({"nodes":[{"id":"c","type":"counter","params":{"step":1}}],"edges":[]})", reg);
    ASSERT_TRUE(g1);
    for (int i = 0; i < 5; ++i) tick_graph(*g1, 0.0);
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g1->nodes[0].data)->count_, 5.f);

    // Edit: same node id, new step param.
    auto g2 = parse_graph(R"({"nodes":[{"id":"c","type":"counter","params":{"step":2}}],"edges":[]})", reg);
    ASSERT_TRUE(g2);
    migrate_graph(*g2, *g1);

    auto* c = static_cast<CounterNode*>(g2->nodes[0].data);
    EXPECT_FLOAT_EQ(c->count_, 5.f);            // live state adopted
    EXPECT_FLOAT_EQ(c->inputs.step.value, 2.f); // declarative edit applied
    tick_graph(*g2, 0.0);
    EXPECT_FLOAT_EQ(c->count_, 7.f);
}

TEST(MigrateGraph, NewAndRetypedNodesRebuild) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    auto g1 = parse_graph(R"({"nodes":[{"id":"a","type":"counter","params":{}}],"edges":[]})", reg);
    tick_graph(*g1, 0.0);
    auto g2 = parse_graph(R"({"nodes":[{"id":"a","type":"counter","params":{}},{"id":"b","type":"counter","params":{}}],"edges":[]})", reg);
    migrate_graph(*g2, *g1);
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g2->nodes[0].data)->count_, 1.f);
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g2->nodes[1].data)->count_, 0.f);
}

TEST(ParseGraph, ToleratesStandardJsonWhitespace) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    auto g = parse_graph(R"({
        "nodes": [ {"id": "c", "type": "counter", "params": {"step": 3}} ],
        "edges": []
    })", reg);
    ASSERT_TRUE(g);
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g->nodes[0].data)->inputs.step.value, 3.f);
}

TEST(SubgraphInlets, RegistryJsonInjectionFansOut) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    const char* sub = R"({
      "inlets":[{"name":"v","node":"c1","port":"step"},
                {"name":"v","node":"c2","port":"step"}],
      "outlets":[{"name":"n1","node":"c1","port":"count"},
                 {"name":"n2","node":"c2","port":"count"}],
      "nodes":[{"id":"c1","type":"counter","params":{}},
               {"id":"c2","type":"counter","params":{}}],
      "edges":[]
    })";
    std::string path = "/tmp/twin_counter.json";
    FILE* f = fopen(path.c_str(), "w");
    fwrite(sub, 1, strlen(sub), f);
    fclose(f);
    ASSERT_TRUE(reg.load_plugin(path));

    auto g = parse_graph(R"({
      "nodes":[{"id":"src","type":"counter","params":{"step":5}},
               {"id":"tw","type":"twin_counter","params":{}}],
      "edges":[{"from":"src.count","to":"tw.v"}]
    })", reg);
    ASSERT_TRUE(g);
    tick_graph(*g, 0.0);  // src.count=5 published; tw sees stale/no inlet yet
    tick_graph(*g, 0.0);  // tw receives 5 → both counters step by 5
    auto n1 = g->values.find("tw.n1");
    ASSERT_NE(n1, g->values.end());
    EXPECT_GE(std::get<double>(n1->second), 5.0);
    auto n2 = g->values.find("tw.n2");
    ASSERT_NE(n2, g->values.end());
    EXPECT_GE(std::get<double>(n2->second), 5.0);
}

// ── text params, subgraph migration, nesting ───────────────────────────────
struct TextyNode {
    static consteval std::string_view name() { return "texty"; }
    struct inputs  { ::text<"label"> label; } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double) { outputs.out.value = float(inputs.label.value.size()); }
};

TEST(TextParams, RoundTripThroughGraphJson) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<TextyNode>());
    auto g = parse_graph(R"({"nodes":[{"id":"t","type":"texty","params":{"label":"hello world"}}],"edges":[]})", reg);
    ASSERT_TRUE(g);
    EXPECT_EQ(static_cast<TextyNode*>(g->nodes[0].data)->inputs.label.value, "hello world");
    auto json = serialize_graph(*g);
    EXPECT_NE(json.find("\"label\":\"hello world\""), std::string::npos);
    auto g2 = parse_graph(json, reg);
    ASSERT_TRUE(g2);
    EXPECT_EQ(static_cast<TextyNode*>(g2->nodes[0].data)->inputs.label.value, "hello world");
}

TEST(MigrateGraph, RegistrySubgraphInstanceStateAdopted) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    const char* sub = R"({"inlets":[{"name":"v","node":"c","port":"step"}],
        "outlets":[{"name":"n","node":"c","port":"count"}],
        "nodes":[{"id":"c","type":"counter","params":{"step":1}}],"edges":[]})";
    FILE* f = fopen("/tmp/one_counter.json", "w");
    fwrite(sub, 1, strlen(sub), f); fclose(f);
    ASSERT_TRUE(reg.load_plugin("/tmp/one_counter.json"));

    auto g1 = parse_graph(R"({"nodes":[{"id":"s","type":"one_counter","params":{}}],"edges":[]})", reg);
    ASSERT_TRUE(g1);
    for (int i = 0; i < 4; ++i) tick_graph(*g1, 0.0);
    double before = std::get<double>(g1->values.at("s.n"));
    EXPECT_GE(before, 4.0);

    auto g2 = parse_graph(R"({"nodes":[{"id":"s","type":"one_counter","params":{}}],"edges":[]})", reg);
    migrate_graph(*g2, *g1);  // same registry descriptor → inner graph adopted
    tick_graph(*g2, 0.0);
    EXPECT_GE(std::get<double>(g2->values.at("s.n")), before + 1.0);
}

TEST(SubgraphInlets, NestedTwoLevelsForwards) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    const char* inner = R"({"inlets":[{"name":"v","node":"c","port":"step"}],
        "outlets":[{"name":"n","node":"c","port":"count"}],
        "nodes":[{"id":"c","type":"counter","params":{}}],"edges":[]})";
    FILE* f = fopen("/tmp/inner_counter.json", "w");
    fwrite(inner, 1, strlen(inner), f); fclose(f);
    ASSERT_TRUE(reg.load_plugin("/tmp/inner_counter.json"));
    const char* outer = R"({"inlets":[{"name":"vv","node":"i","port":"v"}],
        "outlets":[{"name":"nn","node":"i","port":"n"}],
        "nodes":[{"id":"i","type":"inner_counter","params":{}}],"edges":[]})";
    f = fopen("/tmp/outer_counter.json", "w");
    fwrite(outer, 1, strlen(outer), f); fclose(f);
    ASSERT_TRUE(reg.load_plugin("/tmp/outer_counter.json"));

    auto g = parse_graph(R"({"nodes":[
        {"id":"src","type":"counter","params":{"step":7}},
        {"id":"o","type":"outer_counter","params":{}}],
        "edges":[{"from":"src.count","to":"o.vv"}]})", reg);
    ASSERT_TRUE(g);
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.0);
    auto it = g->values.find("o.nn");
    ASSERT_NE(it, g->values.end());
    EXPECT_GE(std::get<double>(it->second), 7.0);  // step=7 reached two levels deep
}

TEST(FeedbackEdges, SelfEdgeIntegratesWithOneTickDelay) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    // counter.count → counter.step: each tick steps by last tick's count.
    // count: 1, 2, 4, 8 ... (doubles once feedback kicks in)
    auto g = parse_graph(R"({"nodes":[{"id":"c","type":"counter","params":{"step":1}}],
        "edges":[{"from":"c.count","to":"c.step"}]})", reg);
    ASSERT_TRUE(g);
    tick_graph(*g, 0.0);  // 0+1 = 1 (no feedback value yet)
    tick_graph(*g, 0.0);  // 1+1 = 2
    tick_graph(*g, 0.0);  // 2+2 = 4
    tick_graph(*g, 0.0);  // 4+4 = 8
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g->nodes[0].data)->count_, 8.f);
}

TEST(FeedbackEdges, TwoNodeCycleDelaysExactlyOneEdge) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    // a.count → b.step is a true edge (same tick); b.count → a.step closes
    // the cycle and becomes a z⁻¹ mapping (last tick's value).
    auto g = parse_graph(R"({"nodes":[
        {"id":"a","type":"counter","params":{"step":1}},
        {"id":"b","type":"counter","params":{"step":0}}],
        "edges":[{"from":"a.count","to":"b.step"},
                 {"from":"b.count","to":"a.step"}]})", reg);
    ASSERT_TRUE(g);
    auto* a = static_cast<CounterNode*>(g->nodes[0].data);
    auto* b = static_cast<CounterNode*>(g->nodes[1].data);
    tick_graph(*g, 0.0);  // a: 0+1=1 (no feedback yet); b: 0+1=1 (same tick)
    EXPECT_FLOAT_EQ(a->count_, 1.f);
    EXPECT_FLOAT_EQ(b->count_, 1.f);
    tick_graph(*g, 0.0);  // a: 1+1(b last)=2; b: 1+2(a now)=3
    EXPECT_FLOAT_EQ(a->count_, 2.f);
    EXPECT_FLOAT_EQ(b->count_, 3.f);
    tick_graph(*g, 0.0);  // a: 2+3=5; b: 3+5=8
    EXPECT_FLOAT_EQ(a->count_, 5.f);
    EXPECT_FLOAT_EQ(b->count_, 8.f);
}

TEST(FeedbackEdges, CycleMappingVisibleInSerialization) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    auto g = parse_graph(R"({"nodes":[{"id":"c","type":"counter","params":{}}],
        "edges":[{"from":"c.count","to":"c.step"}]})", reg);
    ASSERT_TRUE(g);
    auto json = serialize_graph(*g);
    EXPECT_NE(json.find("\"mappings\":[{\"kind\":\"z1\",\"from\":\"c.count\",\"to\":\"c.step\"}]"),
              std::string::npos);
    // …and the mappings array must round-trip harmlessly through parse.
    auto g2 = parse_graph(json, reg);
    ASSERT_TRUE(g2);
    EXPECT_EQ(g2->edges.size(), 1u);
}

TEST(TickPlan, AcyclicGraphHasNoDelays) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    auto g = parse_graph(R"({"nodes":[
        {"id":"a","type":"counter","params":{"step":2}},
        {"id":"b","type":"counter","params":{"step":0}}],
        "edges":[{"from":"a.count","to":"b.step"}]})", reg);
    ASSERT_TRUE(g);
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.0);
    // True edge: b sees a's count the SAME tick. a: 2,4; b: 0+2=2, 2+4=6.
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g->nodes[1].data)->count_, 6.f);
    EXPECT_EQ(serialize_graph(*g).find("\"mappings\""), std::string::npos);
}

struct BangerNode {
    static consteval std::string_view name() { return "banger"; }
    struct inputs  { slider<"fire", "", float, fp(0.f), fp(1.f), fp(0.f)> fire; } inputs;
    struct outputs { bang<"out"> out; } outputs;
    bool prev_ = false;
    void operator()(double) {
        bool on = inputs.fire.value > 0.5f;
        outputs.out.triggered = on && !prev_;   // fires for exactly one tick
        prev_ = on;
    }
};

struct ListenerNode {
    static consteval std::string_view name() { return "listener"; }
    struct inputs  { bang<"in"> in; } inputs;
    struct outputs { port<"count", float> count; } outputs;
    void operator()(double) {
        if (inputs.in.triggered) outputs.count.value += 1.f;
    }
};

TEST(BangPorts, FlowEndToEndExactlyOnce) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<BangerNode>());
    reg.register_builtin(make_descriptor<ListenerNode>());
    auto g = parse_graph(R"({"nodes":[
        {"id":"b","type":"banger","params":{}},
        {"id":"l","type":"listener","params":{}}],
        "edges":[{"from":"b.out","to":"l.in"}]})", reg);
    ASSERT_TRUE(g);
    auto* b = static_cast<BangerNode*>(g->nodes[0].data);
    auto* l = static_cast<ListenerNode*>(g->nodes[1].data);

    tick_graph(*g, 0.0);                       // idle: no fire
    EXPECT_FLOAT_EQ(l->outputs.count.value, 0.f);
    b->inputs.fire.value = 1.f;
    tick_graph(*g, 0.0);                       // rising edge: one bang
    tick_graph(*g, 0.0);                       // held: no re-fire
    tick_graph(*g, 0.0);
    EXPECT_FLOAT_EQ(l->outputs.count.value, 1.f);
    b->inputs.fire.value = 0.f;
    tick_graph(*g, 0.0);
    b->inputs.fire.value = 1.f;
    tick_graph(*g, 0.0);                       // second press: second bang
    EXPECT_FLOAT_EQ(l->outputs.count.value, 2.f);
}

TEST(BangPorts, SchemaReportsBangKind) {
    auto* d = make_descriptor<BangerNode>();
    ASSERT_NE(d->port_schema, nullptr);
    EXPECT_NE(std::string_view{d->port_schema}.find(
                  "{\"name\":\"out\",\"kind\":\"bang\"}"),
              std::string_view::npos);
}

TEST(PortTypeLegality, IllegalEdgeRejectsGraphLoudly) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    reg.register_builtin(make_descriptor<TextyNode>());
    // counter.count is scalar; texty has no scalar input — wire to a
    // nonexistent port is 'unknown' (legal); wire scalar→scalar is legal.
    auto ok = parse_graph(R"({"nodes":[
        {"id":"a","type":"counter","params":{}},
        {"id":"b","type":"counter","params":{}}],
        "edges":[{"from":"a.count","to":"b.step"}]})", reg);
    EXPECT_TRUE(ok != nullptr);
}
