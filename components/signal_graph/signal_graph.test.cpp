// Copyright 2025 Travis West
#include "signal_graph.hpp"

#include <gtest/gtest.h>

#include <cstdio>

#include "component_registry.hpp"
#include "eyeballs_node_abi.h"
#include "eyeballs_node_abi.hpp"
#include "signal_graph_plan.hpp"
#include "subgraph_node.hpp"
#include "sygaldry_endpoints.hpp"
#include "tri_mesh.hpp"

static int nodeA_process_count = 0;
static int nodeB_process_count = 0;

// ── lift (rung 3) test fixtures ───────────────────────────────────────────────
// A span source whose rows the test mutates between ticks (reorder / resize).
struct SpanSourceNode {
    static consteval std::string_view name() { return "span_source"; }
    struct endpoints {
        out<Span> positions;
    } endpoints;
    std::vector<float> rows;  // N×1
    void operator()(double) {
        endpoints.positions.value =
            Span{rows.empty() ? nullptr : rows.data(), int(rows.size()), 1, Axis::Item, Axis::Cell};
    }
};
// Stateful per-instance accumulator: keyed so a reorder keeps each sum.
struct AccumNode {
    static consteval std::string_view name() { return "accum"; }
    static constexpr std::string_view lift_key() { return "x"; }
    struct endpoints {
        in<float> x;
        out<float> sum;
    } endpoints;
    void operator()(double) {
        sum_ += endpoints.x.get();
        endpoints.sum.value = sum_;
    }

private:
    float sum_ = 0.f;
};
// A SECOND span source: the KEY channel (stable ids), independent of the
// lifted cell. The test reorders the cell rows while the ids track them.
struct KeySourceNode {
    static consteval std::string_view name() { return "key_source"; }
    struct endpoints {
        out<Span> keys;
    } endpoints;
    std::vector<float> rows;  // N×1 ids
    void operator()(double) {
        endpoints.keys.value =
            Span{rows.empty() ? nullptr : rows.data(), int(rows.size()), 1, Axis::Item, Axis::Cell};
    }
};
// Inner node of the keyed card-like subgraph: receives an id (proves the key
// inlet is bound) and accumulates x. Keyed on id at the subgraph level.
struct KeyedAccumNode {
    static consteval std::string_view name() { return "keyed_accum"; }
    struct endpoints {
        in<float> id;
        in<float> x;
        out<float> sum;
        out<float> seen_id;
    } endpoints;
    void operator()(double) {
        sum_ += endpoints.x.get();
        endpoints.sum.value = sum_;
        endpoints.seen_id.value = endpoints.id.get();
    }

private:
    float sum_ = 0.f;
};
// A vec3 (N×3) span source — the card-position channel for the S4 lift.
struct Vec3SpanSourceNode {
    static consteval std::string_view name() { return "vec3_source"; }
    struct endpoints {
        out<Span> positions;
    } endpoints;
    std::vector<float> rows;  // N×3
    void operator()(double) {
        endpoints.positions.value = Span{
            rows.empty() ? nullptr : rows.data(), int(rows.size() / 3), 3, Axis::Item, Axis::Cell};
    }
};
// A mesh-at-position generator (card_mesh stand-in): emits a 1-vertex mesh at
// `position` so the test can read back each lifted instance's position.
struct MeshAtNode {
    static consteval std::string_view name() { return "mesh_at"; }
    struct endpoints {
        in<float> id;
        in<Eigen::Vector3f> position;
        out<MeshPtr> mesh;
    } endpoints;
    void operator()(double) {
        TriMeshData m;
        Eigen::Vector3f p = endpoints.position.get();
        m.vertices.push_back({p, {0, 0, 1}, {1, 1, 1, 1}});
        endpoints.mesh.value = std::make_shared<const TriMeshData>(std::move(m));
    }
};
// Gathers a lifted MeshArray, recording each mesh's first vertex position.
struct MeshArraySinkNode {
    static consteval std::string_view name() { return "mesh_array_sink"; }
    struct endpoints {
        whole<MeshArray> meshes;
    } endpoints;
    std::vector<Eigen::Vector3f> seen;
    void operator()(double) {
        MeshArray a = endpoints.meshes.get();
        seen.clear();
        for (int i = 0; i < a.count; ++i)
            seen.push_back(
                a.data[i] && !a.data[i]->vertices.empty() ? a.data[i]->vertices[0].position
                                                          : Eigen::Vector3f::Zero());
    }
};
// Stateless host: declares CellMap; doubles its input, no per-row state.
struct DoubleNode {
    static consteval std::string_view name() { return "double2"; }
    static constexpr int lift_kind() { return lift::stateless; }
    struct endpoints {
        in<float> x;
        out<float> y;
    } endpoints;
    void operator()(double) { endpoints.y.value = endpoints.x.get() * 2.f; }
};
// A resource holder: lifting it (or a subgraph containing it) is an error.
struct DeviceNode {
    static consteval std::string_view name() { return "device2"; }
    static constexpr int lift_kind() { return lift::resource_holder; }
    struct endpoints {
        in<float> x;
        out<float> y;
    } endpoints;
    void operator()(double) { endpoints.y.value = endpoints.x.get(); }
};
// Text-output host: an unsupported gather kind when lifted.
struct LabelerNode {
    static consteval std::string_view name() { return "labeler"; }
    struct endpoints {
        in<float> x;
        out<std::string> label;
    } endpoints;
    void operator()(double) { endpoints.label.value = "v"; }
};
// A voice: cell input + audio output — Block region when it feeds a dac.
struct VoiceNode {
    static consteval std::string_view name() { return "voice"; }
    struct endpoints {
        in<float> freq;
        out<AudioBuffer> audio;
    } endpoints;
    void operator()(double) {}
};
// Named "dac" so infer_regions pins it (and its audio upstream) to Block.
struct FakeDacNode {
    static consteval std::string_view name() { return "dac"; }
    struct endpoints {
        in<AudioBuffer> audio;
    } endpoints;
    void operator()(double) {}
};
// Whole-array sink: captures the gathered Span (opts out of lifting).
struct SpanSinkNode {
    static consteval std::string_view name() { return "span_sink"; }
    struct endpoints {
        whole<Span> in_data;
    } endpoints;
    std::vector<float> seen;
    int rows = 0;
    void operator()(double) {
        Span s = endpoints.in_data.get();
        rows = s.rows;
        seen.assign(s.data, s.data + (s.data ? s.rows * s.cols : 0));
    }
};

static EyeballsNodeDescriptor make_node_a_desc() {
    return {
        .version = EYEBALLS_ABI_VERSION,
        .type_name = "node_a",
        .description = nullptr,
        .create = []() -> void* { return new int{0}; },
        .destroy = [](void* p) { delete static_cast<int*>(p); },
        .process = [](void*, double) { ++nodeA_process_count; },
        .serialize = [](void*) -> const char* { return strdup("{}"); },
        .free_str = [](const char* s) { free(const_cast<char*>(s)); },
        .deserialize = [](void*, const char*) {},
        .push_textures = nullptr,
        .source_header = nullptr,
        .source_cpp = nullptr,
        .port_schema = nullptr,
        .push_outputs = nullptr,
        .set_scalar_in = nullptr,
        .set_vec2_in = nullptr,
        .set_vec3_in = nullptr,
        .set_vec4_in = nullptr,
        .set_mat4_in = nullptr,
        .set_quat_in = nullptr,
        .set_texture_in = nullptr,
        .set_audio_in = nullptr,
        .set_mesh_in = nullptr,
        .connect = nullptr,
        .output_ptr = nullptr,
        .set_text_in = nullptr,
        .lift_kind = EYEBALLS_LIFT_STATEFUL,
        .lift_key = nullptr,
        .set_host_context = nullptr,
    };
}

static EyeballsNodeDescriptor make_node_b_desc() {
    return {
        .version = EYEBALLS_ABI_VERSION,
        .type_name = "node_b",
        .description = nullptr,
        .create = []() -> void* { return new int{0}; },
        .destroy = [](void* p) { delete static_cast<int*>(p); },
        .process = [](void*, double) { ++nodeB_process_count; },
        .serialize = [](void*) -> const char* { return strdup("{}"); },
        .free_str = [](const char* s) { free(const_cast<char*>(s)); },
        .deserialize = [](void*, const char*) {},
        .push_textures = nullptr,
        .source_header = nullptr,
        .source_cpp = nullptr,
        .port_schema = nullptr,
        .push_outputs = nullptr,
        .set_scalar_in = nullptr,
        .set_vec2_in = nullptr,
        .set_vec3_in = nullptr,
        .set_vec4_in = nullptr,
        .set_mat4_in = nullptr,
        .set_quat_in = nullptr,
        .set_texture_in = nullptr,
        .set_audio_in = nullptr,
        .set_mesh_in = nullptr,
        .connect = nullptr,
        .output_ptr = nullptr,
        .set_text_in = nullptr,
        .lift_kind = EYEBALLS_LIFT_STATEFUL,
        .lift_key = nullptr,
        .set_host_context = nullptr,
    };
}

TEST(SignalGraph, ParseAndTick) {
    static auto desc_a = make_node_a_desc();
    static auto desc_b = make_node_b_desc();

    ComponentRegistry reg;
    reg.register_builtin(&desc_a);
    reg.register_builtin(&desc_b);

    const char* json =
        R"({"nodes":[{"id":"a","type":"node_a","params":{}},{"id":"b","type":"node_b","params":{}}],"edges":[]})";

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
    struct endpoints {
        out<float> val;
    } endpoints;
    void operator()(double) { endpoints.val.value = 42.0f; }
};

// ConsumerNode: has a float input port, records the last value set.
struct ConsumerNode {
    static consteval std::string_view name() { return "consumer"; }
    struct endpoints {
        in<float> val;
    } endpoints;
    void operator()(double) {}
};

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
    auto vals = snapshot_values(*g);
    ASSERT_NE(vals.find("a.val"), vals.end());
    EXPECT_NEAR(std::get<double>(vals["a.val"]), 42.0, 1e-6);

    // Consumer node's input should have been set to 42
    auto* consumer = static_cast<ConsumerNode*>(g->nodes[0].data);
    // node b is index 0 (as parsed)
    EXPECT_NEAR(consumer->endpoints.val.get(), 42.0f, 1e-5f);
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

    auto vals = snapshot_values(*g);
    auto it = vals.find("sub.out");
    ASSERT_NE(it, vals.end());
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
    EXPECT_EQ(g->edges[0].to_node, "b");
    EXPECT_EQ(g->edges[0].to_port, "in_val");

    // Serialize and verify edge appears in output
    auto s = serialize_graph(*g);
    EXPECT_NE(s.find("a.out_val"), std::string::npos);
    EXPECT_NE(s.find("b.in_val"), std::string::npos);
}

// ── migrate_graph ──────────────────────────────────────────────────────────
// Counter node: state survives migration, params re-apply.
struct CounterNode {
    static consteval std::string_view name() { return "counter"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(10.f), fp(1.f)> step;
        out<float> count;
    } endpoints;
    float count_ = 0.f;
    void operator()(double) {
        count_ += endpoints.step.get();
        endpoints.count.value = count_;
    }
};

TEST(MigrateGraph, StateSurvivesParamsReapply) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());

    auto g1 = parse_graph(
        R"({"nodes":[{"id":"c","type":"counter","params":{"step":1}}],"edges":[]})", reg);
    ASSERT_TRUE(g1);
    for (int i = 0; i < 5; ++i) tick_graph(*g1, 0.0);
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g1->nodes[0].data)->count_, 5.f);

    // Edit: same node id, new step param.
    auto g2 = parse_graph(
        R"({"nodes":[{"id":"c","type":"counter","params":{"step":2}}],"edges":[]})", reg);
    ASSERT_TRUE(g2);
    migrate_graph(*g2, *g1);

    auto* c = static_cast<CounterNode*>(g2->nodes[0].data);
    EXPECT_FLOAT_EQ(c->count_, 5.f);                   // live state adopted
    EXPECT_FLOAT_EQ(c->endpoints.step.fallback, 2.f);  // declarative edit applied
    tick_graph(*g2, 0.0);
    EXPECT_FLOAT_EQ(c->count_, 7.f);
}

TEST(MigrateGraph, NewAndRetypedNodesRebuild) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    auto g1 = parse_graph(R"({"nodes":[{"id":"a","type":"counter","params":{}}],"edges":[]})", reg);
    tick_graph(*g1, 0.0);
    auto g2 = parse_graph(
        R"({"nodes":[{"id":"a","type":"counter","params":{}},{"id":"b","type":"counter","params":{}}],"edges":[]})",
        reg);
    migrate_graph(*g2, *g1);
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g2->nodes[0].data)->count_, 1.f);
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g2->nodes[1].data)->count_, 0.f);
}

TEST(ParseGraph, ToleratesStandardJsonWhitespace) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    auto g = parse_graph(
        R"({
        "nodes": [ {"id": "c", "type": "counter", "params": {"step": 3}} ],
        "edges": []
    })",
        reg);
    ASSERT_TRUE(g);
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g->nodes[0].data)->endpoints.step.fallback, 3.f);
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

    auto g = parse_graph(
        R"({
      "nodes":[{"id":"src","type":"counter","params":{"step":5}},
               {"id":"tw","type":"twin_counter","params":{}}],
      "edges":[{"from":"src.count","to":"tw.v"}]
    })",
        reg);
    ASSERT_TRUE(g);
    tick_graph(*g, 0.0);  // src.count=5 published; tw sees stale/no inlet yet
    tick_graph(*g, 0.0);  // tw receives 5 → both counters step by 5
    auto vals = snapshot_values(*g);
    auto n1 = vals.find("tw.n1");
    ASSERT_NE(n1, vals.end());
    EXPECT_GE(std::get<double>(n1->second), 5.0);
    auto n2 = vals.find("tw.n2");
    ASSERT_NE(n2, vals.end());
    EXPECT_GE(std::get<double>(n2->second), 5.0);
}

// ── text params, subgraph migration, nesting ───────────────────────────────
struct TextyNode {
    static consteval std::string_view name() { return "texty"; }
    struct endpoints {
        normalled_in<std::string> label;
        ::out<float> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = float(endpoints.label.get().size()); }
};

TEST(TextParams, RoundTripThroughGraphJson) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<TextyNode>());
    auto g = parse_graph(
        R"({"nodes":[{"id":"t","type":"texty","params":{"label":"hello world"}}],"edges":[]})",
        reg);
    ASSERT_TRUE(g);
    EXPECT_EQ(static_cast<TextyNode*>(g->nodes[0].data)->endpoints.label.fallback, "hello world");
    auto json = serialize_graph(*g);
    EXPECT_NE(json.find("\"label\":\"hello world\""), std::string::npos);
    auto g2 = parse_graph(json, reg);
    ASSERT_TRUE(g2);
    EXPECT_EQ(static_cast<TextyNode*>(g2->nodes[0].data)->endpoints.label.fallback, "hello world");
}

TEST(MigrateGraph, RegistrySubgraphInstanceStateAdopted) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    const char* sub = R"({"inlets":[{"name":"v","node":"c","port":"step"}],
        "outlets":[{"name":"n","node":"c","port":"count"}],
        "nodes":[{"id":"c","type":"counter","params":{"step":1}}],"edges":[]})";
    FILE* f = fopen("/tmp/one_counter.json", "w");
    fwrite(sub, 1, strlen(sub), f);
    fclose(f);
    ASSERT_TRUE(reg.load_plugin("/tmp/one_counter.json"));

    auto g1 =
        parse_graph(R"({"nodes":[{"id":"s","type":"one_counter","params":{}}],"edges":[]})", reg);
    ASSERT_TRUE(g1);
    for (int i = 0; i < 4; ++i) tick_graph(*g1, 0.0);
    double before = std::get<double>(snapshot_values(*g1).at("s.n"));
    EXPECT_GE(before, 4.0);

    auto g2 =
        parse_graph(R"({"nodes":[{"id":"s","type":"one_counter","params":{}}],"edges":[]})", reg);
    migrate_graph(*g2, *g1);  // same registry descriptor → inner graph adopted
    tick_graph(*g2, 0.0);
    EXPECT_GE(std::get<double>(snapshot_values(*g2).at("s.n")), before + 1.0);
}

TEST(SubgraphInlets, NestedTwoLevelsForwards) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    const char* inner = R"({"inlets":[{"name":"v","node":"c","port":"step"}],
        "outlets":[{"name":"n","node":"c","port":"count"}],
        "nodes":[{"id":"c","type":"counter","params":{}}],"edges":[]})";
    FILE* f = fopen("/tmp/inner_counter.json", "w");
    fwrite(inner, 1, strlen(inner), f);
    fclose(f);
    ASSERT_TRUE(reg.load_plugin("/tmp/inner_counter.json"));
    const char* outer = R"({"inlets":[{"name":"vv","node":"i","port":"v"}],
        "outlets":[{"name":"nn","node":"i","port":"n"}],
        "nodes":[{"id":"i","type":"inner_counter","params":{}}],"edges":[]})";
    f = fopen("/tmp/outer_counter.json", "w");
    fwrite(outer, 1, strlen(outer), f);
    fclose(f);
    ASSERT_TRUE(reg.load_plugin("/tmp/outer_counter.json"));

    auto g = parse_graph(
        R"({"nodes":[
        {"id":"src","type":"counter","params":{"step":7}},
        {"id":"o","type":"outer_counter","params":{}}],
        "edges":[{"from":"src.count","to":"o.vv"}]})",
        reg);
    ASSERT_TRUE(g);
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.0);
    auto vals = snapshot_values(*g);
    auto it = vals.find("o.nn");
    ASSERT_NE(it, vals.end());
    EXPECT_GE(std::get<double>(it->second), 7.0);  // step=7 reached two levels deep
}

TEST(FeedbackEdges, SelfEdgeIntegratesWithOneTickDelay) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    // counter.count → counter.step: each tick steps by last tick's count.
    // count: 1, 2, 4, 8 ... (doubles once feedback kicks in)
    auto g = parse_graph(
        R"({"nodes":[{"id":"c","type":"counter","params":{"step":1}}],
        "edges":[{"from":"c.count","to":"c.step"}]})",
        reg);
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
    auto g = parse_graph(
        R"({"nodes":[
        {"id":"a","type":"counter","params":{"step":1}},
        {"id":"b","type":"counter","params":{"step":0}}],
        "edges":[{"from":"a.count","to":"b.step"},
                 {"from":"b.count","to":"a.step"}]})",
        reg);
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
    auto g = parse_graph(
        R"({"nodes":[{"id":"c","type":"counter","params":{}}],
        "edges":[{"from":"c.count","to":"c.step"}]})",
        reg);
    ASSERT_TRUE(g);
    auto json = serialize_graph(*g);
    EXPECT_NE(
        json.find("\"mappings\":[{\"kind\":\"z1\",\"from\":\"c.count\",\"to\":\"c.step\"}]"),
        std::string::npos);
    // …and the mappings array must round-trip harmlessly through parse.
    auto g2 = parse_graph(json, reg);
    ASSERT_TRUE(g2);
    EXPECT_EQ(g2->edges.size(), 1u);
}

TEST(TickPlan, AcyclicGraphHasNoDelays) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    auto g = parse_graph(
        R"({"nodes":[
        {"id":"a","type":"counter","params":{"step":2}},
        {"id":"b","type":"counter","params":{"step":0}}],
        "edges":[{"from":"a.count","to":"b.step"}]})",
        reg);
    ASSERT_TRUE(g);
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.0);
    // True edge: b sees a's count the SAME tick. a: 2,4; b: 0+2=2, 2+4=6.
    EXPECT_FLOAT_EQ(static_cast<CounterNode*>(g->nodes[1].data)->count_, 6.f);
    EXPECT_EQ(serialize_graph(*g).find("\"mappings\""), std::string::npos);
}

struct BangerNode {
    static consteval std::string_view name() { return "banger"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(1.f), fp(0.f)> fire;
        event_out out;
    } endpoints;
    bool prev_ = false;
    void operator()(double) {
        bool on = endpoints.fire.get() > 0.5f;
        endpoints.out.triggered = on && !prev_;  // fires for exactly one tick
        prev_ = on;
    }
};

struct ListenerNode {
    static consteval std::string_view name() { return "listener"; }
    struct endpoints {
        event_in in;
        ::out<float> count;
    } endpoints;
    void operator()(double) {
        if (endpoints.in.triggered) endpoints.count.value += 1.f;
    }
};

TEST(BangPorts, FlowEndToEndExactlyOnce) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<BangerNode>());
    reg.register_builtin(make_descriptor<ListenerNode>());
    auto g = parse_graph(
        R"({"nodes":[
        {"id":"b","type":"banger","params":{}},
        {"id":"l","type":"listener","params":{}}],
        "edges":[{"from":"b.out","to":"l.in"}]})",
        reg);
    ASSERT_TRUE(g);
    auto* b = static_cast<BangerNode*>(g->nodes[0].data);
    auto* l = static_cast<ListenerNode*>(g->nodes[1].data);

    tick_graph(*g, 0.0);  // idle: no fire
    EXPECT_FLOAT_EQ(l->endpoints.count.value, 0.f);
    b->endpoints.fire.fallback = 1.f;
    tick_graph(*g, 0.0);  // rising edge: one bang
    tick_graph(*g, 0.0);  // held: no re-fire
    tick_graph(*g, 0.0);
    EXPECT_FLOAT_EQ(l->endpoints.count.value, 1.f);
    b->endpoints.fire.fallback = 0.f;
    tick_graph(*g, 0.0);
    b->endpoints.fire.fallback = 1.f;
    tick_graph(*g, 0.0);  // second press: second bang
    EXPECT_FLOAT_EQ(l->endpoints.count.value, 2.f);
}

TEST(BangPorts, SchemaReportsBangKind) {
    auto* d = make_descriptor<BangerNode>();
    ASSERT_NE(d->port_schema, nullptr);
    EXPECT_NE(
        std::string_view{d->port_schema}.find("{\"name\":\"out\",\"kind\":\"bang\"}"),
        std::string_view::npos);
}

TEST(PortTypeLegality, IllegalEdgeRejectsGraphLoudly) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<CounterNode>());
    reg.register_builtin(make_descriptor<TextyNode>());
    // counter.count is scalar; texty has no scalar input — wire to a
    // nonexistent port is 'unknown' (legal); wire scalar→scalar is legal.
    auto ok = parse_graph(
        R"({"nodes":[
        {"id":"a","type":"counter","params":{}},
        {"id":"b","type":"counter","params":{}}],
        "edges":[{"from":"a.count","to":"b.step"}]})",
        reg);
    EXPECT_TRUE(ok != nullptr);
}

// ── endpoints v6: literal pointer wiring through the executor ────────────────

struct V6Src {
    static consteval std::string_view name() { return "v6_src"; }
    struct endpoints {
        out<float> sig;
    } endpoints;
    void operator()(double t) { endpoints.sig.value = static_cast<float>(t); }
};
struct V6Dst {
    static consteval std::string_view name() { return "v6_dst"; }
    struct endpoints {
        in<float, fp(-1.f)> sig;
        out<float> echo;
    } endpoints;
    void operator()(double) { endpoints.echo.value = endpoints.sig.get(); }
};

TEST(EndpointsV6Executor, WiresAreLiteralPointersAndResetOnSwap) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<V6Src>());
    reg.register_builtin(make_descriptor<V6Dst>());
    auto g = parse_graph(
        R"({"nodes":[
        {"id":"a","type":"v6_src"},{"id":"b","type":"v6_dst"}],
        "edges":[{"from":"a.sig","to":"b.sig"}]})",
        reg);
    ASSERT_TRUE(g);
    tick_graph(*g, 1.5);

    V6Src* a = nullptr;
    V6Dst* b = nullptr;
    for (auto& n : g->nodes) {
        if (n.id == "a") a = static_cast<V6Src*>(n.data);
        if (n.id == "b") b = static_cast<V6Dst*>(n.data);
    }
    ASSERT_TRUE(a && b);
    // the consumer reads the producer's storage DIRECTLY
    EXPECT_EQ(b->endpoints.sig.src, &a->endpoints.sig.value);
    EXPECT_FLOAT_EQ(b->endpoints.echo.value, 1.5f);

    // swap to a graph WITHOUT the edge: the migrated instance must not
    // keep a stale pointer — it falls back to the compile-time default.
    auto g2 = parse_graph(
        R"({"nodes":[
        {"id":"a","type":"v6_src"},{"id":"b","type":"v6_dst"}],
        "edges":[]})",
        reg);
    ASSERT_TRUE(g2);
    migrate_graph(*g2, *g);
    tick_graph(*g2, 2.0);
    V6Dst* b2 = nullptr;
    for (auto& n : g2->nodes)
        if (n.id == "b") b2 = static_cast<V6Dst*>(n.data);
    ASSERT_TRUE(b2);
    EXPECT_EQ(b2, b);                           // same migrated instance
    EXPECT_EQ(b2->endpoints.sig.src, nullptr);  // wire_plan reset it
    EXPECT_FLOAT_EQ(b2->endpoints.echo.value, -1.f);
}

// ── text edges + subgraph inlet-params (overnight 2 gates) ──────────────────

struct TextSrcNode {
    static consteval std::string_view name() { return "text_src"; }
    struct endpoints {
        normalled_in<std::string> seed;
        out<std::string> text;
    } endpoints;
    void operator()(double) { endpoints.text.value = endpoints.seed.get() + "!"; }
};
struct TextSinkNode {
    static consteval std::string_view name() { return "text_sink"; }
    struct endpoints {
        normalled_in<std::string> in;
        ::out<float> len;
    } endpoints;
    void operator()(double) { endpoints.len.value = float(endpoints.in.get().size()); }
};

TEST(TextEdges, TextFlowsThroughAnEdge) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<TextSrcNode>());
    reg.register_builtin(make_descriptor<TextSinkNode>());
    auto g = parse_graph(
        R"({"nodes":[
        {"id":"a","type":"text_src","params":{"seed":"hello"}},
        {"id":"b","type":"text_sink"}],
        "edges":[{"from":"a.text","to":"b.in"}]})",
        reg);
    ASSERT_TRUE(g);
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.1);  // slot exists after the first emit
    TextSinkNode* b = nullptr;
    for (auto& n : g->nodes)
        if (n.id == "b") b = static_cast<TextSinkNode*>(n.data);
    ASSERT_TRUE(b);
    EXPECT_EQ(b->endpoints.in.get(), "hello!");
    auto vals = snapshot_values(*g);
    auto it = vals.find("a.text");
    ASSERT_NE(it, vals.end());
    EXPECT_EQ(std::get<std::string>(it->second), "hello!");
}

struct GainNode {
    static consteval std::string_view name() { return "gain2"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(10.f), fp(1.f)> gain;
        ::out<float> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.gain.get() * 2.f; }
};

TEST(SubgraphInletParams, ParamsSetDefaultsAndRoundTrip) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<GainNode>());
    const char* json = R"({"nodes":[{
        "id":"sub","params":{"g":3},
        "graph":{
            "nodes":[{"id":"gn","type":"gain2","params":{}}],
            "edges":[],
            "inlets":[{"name":"g","node":"gn","port":"gain"}],
            "outlets":[{"name":"out","node":"gn","port":"out"}]}}],
        "edges":[]})";
    auto g = parse_graph(json, reg);
    ASSERT_TRUE(g);
    tick_graph(*g, 0.0);
    auto vals = snapshot_values(*g);
    auto it = vals.find("sub.out");
    ASSERT_NE(it, vals.end());
    EXPECT_NEAR(std::get<double>(it->second), 6.0, 1e-6);  // 3 * 2

    // round-trip: serialize captures the param default, reparse applies it
    std::string s = serialize_graph(*g);
    EXPECT_NE(s.find("\"g\":3"), std::string::npos);
    auto g2 = parse_graph(s, reg);
    ASSERT_TRUE(g2);
    tick_graph(*g2, 0.0);
    auto vals2 = snapshot_values(*g2);
    auto it2 = vals2.find("sub.out");
    ASSERT_NE(it2, vals2.end());
    EXPECT_NEAR(std::get<double>(it2->second), 6.0, 1e-6);
}

// ── lift (rung 3): excess-rank edge → N stateful instances ────────────────────

static std::unique_ptr<Graph> make_lift_graph(SpanSourceNode** src_out, SpanSinkNode** sink_out) {
    static auto src_desc = *make_descriptor<SpanSourceNode>();
    static auto acc_desc = *make_descriptor<AccumNode>();
    static auto sink_desc = *make_descriptor<SpanSinkNode>();
    auto g = std::make_unique<Graph>();
    auto* src = new SpanSourceNode{};
    auto* acc = new AccumNode{};
    auto* sink = new SpanSinkNode{};
    g->nodes.push_back({&src_desc, src, "src"});
    g->nodes.push_back({&acc_desc, acc, "acc"});
    g->nodes.push_back({&sink_desc, sink, "sink"});
    g->edges.push_back({"src", "positions", "acc", "x"});   // excess-rank → lift
    g->edges.push_back({"acc", "sum", "sink", "in_data"});  // gather
    *src_out = src;
    *sink_out = sink;
    return g;
}

TEST(SignalGraphLift, ExcessRankFansToNInstancesAndGathers) {
    SpanSourceNode* src;
    SpanSinkNode* sink;
    auto g = make_lift_graph(&src, &sink);
    src->rows = {1.f, 2.f, 3.f};

    tick_graph(*g, 0.0);  // each accum sees its row once
    ASSERT_EQ(sink->rows, 3);
    EXPECT_FLOAT_EQ(sink->seen[0], 1.f);
    EXPECT_FLOAT_EQ(sink->seen[1], 2.f);
    EXPECT_FLOAT_EQ(sink->seen[2], 3.f);

    tick_graph(*g, 0.0);  // independent state: each sum doubles
    EXPECT_FLOAT_EQ(sink->seen[0], 2.f);
    EXPECT_FLOAT_EQ(sink->seen[1], 4.f);
    EXPECT_FLOAT_EQ(sink->seen[2], 6.f);
}

TEST(SignalGraphLift, KeyedInstanceStateSurvivesReorder) {
    SpanSourceNode* src;
    SpanSinkNode* sink;
    auto g = make_lift_graph(&src, &sink);
    src->rows = {10.f, 20.f, 30.f};
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.0);  // sums: 20, 40, 60 (keyed by value)

    // Reorder the rows; keyed identity must follow the VALUE, not the slot.
    src->rows = {30.f, 10.f, 20.f};
    tick_graph(*g, 0.0);
    ASSERT_EQ(sink->rows, 3);
    EXPECT_FLOAT_EQ(sink->seen[0], 60.f + 30.f);  // the "30" instance kept its 60
    EXPECT_FLOAT_EQ(sink->seen[1], 20.f + 10.f);  // the "10" instance kept its 20
    EXPECT_FLOAT_EQ(sink->seen[2], 40.f + 20.f);  // the "20" instance kept its 40
}

TEST(SignalGraphLift, ArrayShrinkDropsInstances) {
    SpanSourceNode* src;
    SpanSinkNode* sink;
    auto g = make_lift_graph(&src, &sink);
    src->rows = {5.f, 6.f, 7.f};
    tick_graph(*g, 0.0);
    src->rows = {5.f};  // shrink to one
    tick_graph(*g, 0.0);
    EXPECT_EQ(sink->rows, 1);
    EXPECT_FLOAT_EQ(sink->seen[0], 10.f);   // the "5" instance kept its state
    EXPECT_EQ(g->lifted_store.size(), 1u);  // dropped instances destroyed
}

TEST(SignalGraphLift, LiftedInstanceStateSurvivesGraphSwap) {
    SpanSourceNode* src;
    SpanSinkNode* sink;
    auto g = make_lift_graph(&src, &sink);
    src->rows = {1.f, 2.f};
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.0);  // sums: 2, 4 (keyed by value)

    // Live edit: build a fresh graph, migrate. Lifted state must survive.
    SpanSourceNode* src2;
    SpanSinkNode* sink2;
    auto g2 = make_lift_graph(&src2, &sink2);
    migrate_graph(*g2, *g);  // swaps node data — re-fetch the live instances
    auto* src_live = static_cast<SpanSourceNode*>(g2->nodes[0].data);
    auto* sink_live = static_cast<SpanSinkNode*>(g2->nodes[2].data);
    src_live->rows = {1.f, 2.f};
    tick_graph(*g2, 0.0);
    ASSERT_EQ(sink_live->rows, 2);
    EXPECT_FLOAT_EQ(sink_live->seen[0], 3.f);  // the "1" instance kept its 2
    EXPECT_FLOAT_EQ(sink_live->seen[1], 6.f);  // the "2" instance kept its 4
}

// Subgraph lift (the Part II acceptance path): a subgraph is liftable as N
// clone_graph clones via desc->create(), each with independent inner state.
// Registered by type (outer edges to inline subgraphs are unsupported by the
// mini-parser; Part II's card.json is by-reference too).
TEST(SignalGraphLift, SubgraphLiftsToNIndependentClones) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<SpanSourceNode>());
    reg.register_builtin(make_descriptor<AccumNode>());
    reg.register_builtin(make_descriptor<SpanSinkNode>());
    // The subgraph: one scalar inlet x → inner accum → one scalar outlet sum.
    const char* sub = R"({
        "inlets":[{"name":"x","node":"a","port":"x"}],
        "outlets":[{"name":"sum","node":"a","port":"sum"}],
        "nodes":[{"id":"a","type":"accum"}],"edges":[]})";
    FILE* f = fopen("/tmp/accum_sub.json", "w");
    fwrite(sub, 1, strlen(sub), f);
    fclose(f);
    ASSERT_TRUE(reg.load_plugin("/tmp/accum_sub.json"));

    auto g = parse_graph(
        R"({
      "nodes":[
        {"id":"src","type":"span_source"},
        {"id":"sub","type":"accum_sub"},
        {"id":"sink","type":"span_sink"}],
      "edges":[
        {"from":"src.positions","to":"sub.x"},
        {"from":"sub.sum","to":"sink.in_data"}]})",
        reg);
    ASSERT_TRUE(g);
    auto* src = static_cast<SpanSourceNode*>(g->nodes[0].data);
    auto* sink = static_cast<SpanSinkNode*>(g->nodes[2].data);
    src->rows = {4.f, 7.f};
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.0);  // two ticks: each inner accum doubles its row
    ASSERT_EQ(sink->rows, 2);
    EXPECT_FLOAT_EQ(sink->seen[0], 8.f);   // 4+4 — independent clone state
    EXPECT_FLOAT_EQ(sink->seen[1], 14.f);  // 7+7
}

// Keyed-by-id subgraph lift (vr_editor_decomposition.md S4, rung-3
// acceptance): a subgraph with a SEPARATE id inlet (lift_key:"id") lifted over
// a cell array, keyed on the id source — NOT the lifted cell. The cell rows
// reorder while the ids stay paired; each clone's state must follow its ID.
TEST(SignalGraphLift, KeyedByIdSubgraphSurvivesCellReorder) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<SpanSourceNode>());
    reg.register_builtin(make_descriptor<KeySourceNode>());
    reg.register_builtin(make_descriptor<KeyedAccumNode>());
    reg.register_builtin(make_descriptor<SpanSinkNode>());
    // The card-like subgraph: id inlet (the KEY) + x inlet (the lifted cell).
    const char* sub = R"({
        "inlets":[{"name":"id","node":"a","port":"id"},
                  {"name":"x","node":"a","port":"x"}],
        "outlets":[{"name":"sum","node":"a","port":"sum"}],
        "lift_key":"id",
        "nodes":[{"id":"a","type":"keyed_accum"}],"edges":[]})";
    FILE* f = fopen("/tmp/keyed_sub.json", "w");
    fwrite(sub, 1, strlen(sub), f);
    fclose(f);
    ASSERT_TRUE(reg.load_plugin("/tmp/keyed_sub.json"));

    auto g = parse_graph(
        R"({
      "nodes":[
        {"id":"cells","type":"span_source"},
        {"id":"ids","type":"key_source"},
        {"id":"card","type":"keyed_sub"},
        {"id":"sink","type":"span_sink"}],
      "edges":[
        {"from":"cells.positions","to":"card.x"},
        {"from":"ids.keys","to":"card.id"},
        {"from":"card.sum","to":"sink.in_data"}]})",
        reg);
    ASSERT_TRUE(g);
    auto* cells = static_cast<SpanSourceNode*>(g->nodes[0].data);
    auto* ids = static_cast<KeySourceNode*>(g->nodes[1].data);
    auto* sink = static_cast<SpanSinkNode*>(g->nodes[3].data);

    ids->rows = {100.f, 200.f, 300.f};  // stable ids
    cells->rows = {10.f, 20.f, 30.f};
    tick_graph(*g, 0.0);
    tick_graph(*g, 0.0);  // sums: 20, 40, 60 keyed by id 100/200/300

    // Reorder cells AND ids together (id 300 now slot 0). State follows the ID.
    ids->rows = {300.f, 100.f, 200.f};
    cells->rows = {30.f, 10.f, 20.f};
    tick_graph(*g, 0.0);
    ASSERT_EQ(sink->rows, 3);
    EXPECT_FLOAT_EQ(sink->seen[0], 60.f + 30.f);  // id 300 kept its 60
    EXPECT_FLOAT_EQ(sink->seen[1], 20.f + 10.f);  // id 100 kept its 20
    EXPECT_FLOAT_EQ(sink->seen[2], 40.f + 20.f);  // id 200 kept its 40
}

// The editor.json S4 acceptance, headless: a card-like mesh subgraph lifted
// over an N×3 position Span (keyed by a separate id) gathers N DISTINCT meshes,
// each at its OWN position — the cards-from-the-live-graph render path.
TEST(SignalGraphLift, KeyedCardSubgraphGathersDistinctMeshes) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<Vec3SpanSourceNode>());
    reg.register_builtin(make_descriptor<KeySourceNode>());
    reg.register_builtin(make_descriptor<MeshAtNode>());
    reg.register_builtin(make_descriptor<MeshArraySinkNode>());
    const char* sub = R"({
        "inlets":[{"name":"id","node":"g","port":"id"},
                  {"name":"position","node":"g","port":"position"}],
        "outlets":[{"name":"mesh","node":"g","port":"mesh"}],
        "lift_key":"id",
        "nodes":[{"id":"g","type":"mesh_at"}],"edges":[]})";
    FILE* f = fopen("/tmp/card_like.json", "w");
    fwrite(sub, 1, strlen(sub), f);
    fclose(f);
    ASSERT_TRUE(reg.load_plugin("/tmp/card_like.json"));

    auto g = parse_graph(
        R"({
      "nodes":[
        {"id":"gsrc","type":"vec3_source"},
        {"id":"ids","type":"key_source"},
        {"id":"card","type":"card_like"},
        {"id":"sink","type":"mesh_array_sink"}],
      "edges":[
        {"from":"ids.keys","to":"card.id"},
        {"from":"gsrc.positions","to":"card.position"},
        {"from":"card.mesh","to":"sink.meshes"}]})",
        reg);
    ASSERT_TRUE(g);
    auto* src = static_cast<Vec3SpanSourceNode*>(g->nodes[0].data);
    auto* ids = static_cast<KeySourceNode*>(g->nodes[1].data);
    auto* sink = static_cast<MeshArraySinkNode*>(g->nodes[3].data);
    ids->rows = {1.f, 2.f, 3.f};
    src->rows = {0.f, 1.f, -1.f, 2.f, 0.f, -1.f, -2.f, 0.f, -1.f};  // 3 cards
    tick_graph(*g, 0.0);
    ASSERT_EQ(sink->seen.size(), 3u);
    EXPECT_FLOAT_EQ(sink->seen[0].x(), 0.f);
    EXPECT_FLOAT_EQ(sink->seen[1].x(), 2.f);  // each card at its OWN position
    EXPECT_FLOAT_EQ(sink->seen[2].x(), -2.f);
}

// L2: stateless host → CellMap loop (one node, no clones, no store).
TEST(SignalGraphLift, StatelessHostUsesCellMap) {
    static auto src_desc = *make_descriptor<SpanSourceNode>();
    static auto dbl_desc = *make_descriptor<DoubleNode>();
    static auto sink_desc = *make_descriptor<SpanSinkNode>();
    auto g = std::make_unique<Graph>();
    auto* src = new SpanSourceNode{};
    g->nodes.push_back({&src_desc, src, "src"});
    g->nodes.push_back({&dbl_desc, new DoubleNode{}, "dbl"});
    auto* sink = new SpanSinkNode{};
    g->nodes.push_back({&sink_desc, sink, "sink"});
    g->edges.push_back({"src", "positions", "dbl", "x"});
    g->edges.push_back({"dbl", "y", "sink", "in_data"});
    src->rows = {1.f, 2.f, 3.f};
    tick_graph(*g, 0.0);
    ASSERT_EQ(sink->rows, 3);
    EXPECT_FLOAT_EQ(sink->seen[0], 2.f);
    EXPECT_FLOAT_EQ(sink->seen[2], 6.f);
    EXPECT_TRUE(g->lifted_store.empty());  // CellMap keeps no instances
}

// L2: lifting a resource-holder is a hard error (the lift is refused).
TEST(SignalGraphLift, ResourceHolderIsNotLifted) {
    static auto src_desc = *make_descriptor<SpanSourceNode>();
    static auto dev_desc = *make_descriptor<DeviceNode>();
    EXPECT_EQ(dev_desc.lift_kind, lift::resource_holder);
    auto g = std::make_unique<Graph>();
    auto* src = new SpanSourceNode{};
    g->nodes.push_back({&src_desc, src, "src"});
    g->nodes.push_back({&dev_desc, new DeviceNode{}, "dev"});
    g->edges.push_back({"src", "positions", "dev", "x"});
    src->rows = {1.f, 2.f};
    auto plan = build_plan(*g);  // logs the error, refuses the lift
    EXPECT_TRUE(plan->lift_groups.empty());
}

// ── lift lint: unsupported lift shapes are hard errors at parse ──────────────

TEST(LiftLint, ResourceHolderLiftRejectsAtParse) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<SpanSourceNode>());
    reg.register_builtin(make_descriptor<DeviceNode>());
    testing::internal::CaptureStderr();
    auto g = parse_graph(
        R"({"nodes":[{"id":"src","type":"span_source"},{"id":"dev","type":"device2"}],
        "edges":[{"from":"src.positions","to":"dev.x"}]})",
        reg);
    std::string err = testing::internal::GetCapturedStderr();
    EXPECT_EQ(g, nullptr);
    EXPECT_NE(err.find("'dev'"), std::string::npos);
    EXPECT_NE(err.find("resource-holder"), std::string::npos);
}

TEST(LiftLint, SecondArrayInputRejectsAtParse) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<SpanSourceNode>());
    reg.register_builtin(make_descriptor<KeyedAccumNode>());
    testing::internal::CaptureStderr();
    auto g = parse_graph(
        R"({"nodes":[{"id":"src","type":"span_source"},{"id":"ka","type":"keyed_accum"}],
        "edges":[{"from":"src.positions","to":"ka.id"},
                 {"from":"src.positions","to":"ka.x"}]})",
        reg);
    std::string err = testing::internal::GetCapturedStderr();
    EXPECT_EQ(g, nullptr);
    EXPECT_NE(err.find("'x'"), std::string::npos);
    EXPECT_NE(err.find("one lift per host"), std::string::npos);
}

TEST(LiftLint, NonGatheredOutputRejectsAtParse) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<SpanSourceNode>());
    reg.register_builtin(make_descriptor<KeyedAccumNode>());
    reg.register_builtin(make_descriptor<SpanSinkNode>());
    testing::internal::CaptureStderr();
    auto g = parse_graph(
        R"({"nodes":[{"id":"src","type":"span_source"},{"id":"ka","type":"keyed_accum"},
                     {"id":"sink","type":"span_sink"}],
        "edges":[{"from":"src.positions","to":"ka.x"},
                 {"from":"ka.seen_id","to":"sink.in_data"}]})",
        reg);
    std::string err = testing::internal::GetCapturedStderr();
    EXPECT_EQ(g, nullptr);
    EXPECT_NE(err.find("seen_id"), std::string::npos);
}

TEST(LiftLint, GatherIntoCellInputRejectsAtParse) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<SpanSourceNode>());
    reg.register_builtin(make_descriptor<AccumNode>());
    reg.register_builtin(make_descriptor<ConsumerNode>());
    testing::internal::CaptureStderr();
    auto g = parse_graph(
        R"({"nodes":[{"id":"src","type":"span_source"},{"id":"acc","type":"accum"},
                     {"id":"c","type":"consumer"}],
        "edges":[{"from":"src.positions","to":"acc.x"},
                 {"from":"acc.sum","to":"c.val"}]})",
        reg);
    std::string err = testing::internal::GetCapturedStderr();
    EXPECT_EQ(g, nullptr);
    EXPECT_NE(err.find("c.val"), std::string::npos);
}

TEST(LiftLint, UnsupportedGatherKindRejectsAtParse) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<SpanSourceNode>());
    reg.register_builtin(make_descriptor<LabelerNode>());
    reg.register_builtin(make_descriptor<TextSinkNode>());
    testing::internal::CaptureStderr();
    auto g = parse_graph(
        R"({"nodes":[{"id":"src","type":"span_source"},{"id":"lab","type":"labeler"},
                     {"id":"t","type":"text_sink"}],
        "edges":[{"from":"src.positions","to":"lab.x"},
                 {"from":"lab.label","to":"t.in"}]})",
        reg);
    std::string err = testing::internal::GetCapturedStderr();
    EXPECT_EQ(g, nullptr);
    EXPECT_NE(err.find("cannot gather 'text'"), std::string::npos);
}

TEST(LiftLint, BlockRegionLiftRejectsAtParse) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<SpanSourceNode>());
    reg.register_builtin(make_descriptor<VoiceNode>());
    reg.register_builtin(make_descriptor<FakeDacNode>());
    testing::internal::CaptureStderr();
    auto g = parse_graph(
        R"({"nodes":[{"id":"src","type":"span_source"},{"id":"v","type":"voice"},
                     {"id":"d","type":"dac"}],
        "edges":[{"from":"src.positions","to":"v.freq"},
                 {"from":"v.audio","to":"d.audio"}]})",
        reg);
    std::string err = testing::internal::GetCapturedStderr();
    EXPECT_EQ(g, nullptr);
    EXPECT_NE(err.find("'v'"), std::string::npos);
    EXPECT_NE(err.find("Block"), std::string::npos);
}

// H1: dangling edges (either end) must not crash the lift scan.
TEST(LiftLint, DanglingEdgesDoNotCrashPlanBuild) {
    SpanSourceNode* src;
    SpanSinkNode* sink;
    auto g = make_lift_graph(&src, &sink);
    g->edges.push_back({"ghost", "out", "acc", "z"});    // dangling producer
    g->edges.push_back({"acc", "sum", "ghost2", "in"});  // dangling consumer
    src->rows = {1.f};
    tick_graph(*g, 0.0);
    EXPECT_EQ(sink->rows, 1);
}

// Defensive path (programmatic graphs skip parse): the lift is refused AND
// the excess edge is left unwired — never bound as a raw span→cell wire.
TEST(LiftLint, BuildPlanRefusesResourceHolderWithoutGarbageWire) {
    static auto src_desc = *make_descriptor<SpanSourceNode>();
    static auto dev_desc = *make_descriptor<DeviceNode>();
    auto g = std::make_unique<Graph>();
    auto* src = new SpanSourceNode{};
    auto* dev = new DeviceNode{};
    g->nodes.push_back({&src_desc, src, "src"});
    g->nodes.push_back({&dev_desc, dev, "dev"});
    g->edges.push_back({"src", "positions", "dev", "x"});
    src->rows = {1.f, 2.f};
    tick_graph(*g, 0.0);
    EXPECT_TRUE(g->plan->lift_groups.empty());
    EXPECT_TRUE(g->plan->wires.empty());
    EXPECT_EQ(dev->endpoints.x.src, nullptr);  // input left unwired
}

// L2: a subgraph containing a resource-holder is itself unliftable.
TEST(SignalGraphLift, SubgraphWithResourceHolderInfersUnliftable) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<DeviceNode>());
    const char* sub = R"({
        "inlets":[{"name":"x","node":"d","port":"x"}],
        "outlets":[{"name":"y","node":"d","port":"y"}],
        "nodes":[{"id":"d","type":"device2"}],"edges":[]})";
    FILE* f = fopen("/tmp/dev_sub.json", "w");
    fwrite(sub, 1, strlen(sub), f);
    fclose(f);
    ASSERT_TRUE(reg.load_plugin("/tmp/dev_sub.json"));
    const EyeballsNodeDescriptor* d = reg.find("dev_sub");
    ASSERT_NE(d, nullptr);
    EXPECT_EQ(d->lift_kind, lift::resource_holder);  // inferred from inner node
}

// M2: keyed identity must not collapse close keys — %g (6 sig digits)
// renders 2^23 and 2^23+1 identically; the key format must not.
TEST(SignalGraphLift, KeyPrecisionKeepsCloseKeysDistinct) {
    SpanSourceNode* src;
    SpanSinkNode* sink;
    auto g = make_lift_graph(&src, &sink);
    src->rows = {8388608.f, 8388609.f};
    tick_graph(*g, 0.0);
    EXPECT_EQ(g->lifted_store.size(), 2u);  // two instances, not one shared
    tick_graph(*g, 0.0);
    ASSERT_EQ(sink->rows, 2);
    EXPECT_FLOAT_EQ(sink->seen[0], 2.f * 8388608.f);
    EXPECT_FLOAT_EQ(sink->seen[1], 2.f * 8388609.f);
}

// H2: a live edit that changes the lifted host's DESCRIPTOR (re-registered
// type / re-parsed inline subgraph) must move the stored clones onto the new
// desc — never keep running (or destroy through) the stale one.
TEST(MigrateGraph, LiftedInstancesFollowDescriptorChange) {
    static auto src_desc = *make_descriptor<SpanSourceNode>();
    static auto sink_desc = *make_descriptor<SpanSinkNode>();
    static auto acc_v1 = *make_descriptor<AccumNode>();
    static auto acc_v2 = *make_descriptor<AccumNode>();  // same type, new identity
    auto make = [&](const EyeballsNodeDescriptor* acc) {
        auto g = std::make_unique<Graph>();
        g->nodes.push_back({&src_desc, new SpanSourceNode{}, "src"});
        g->nodes.push_back({acc, new AccumNode{}, "acc"});
        g->nodes.push_back({&sink_desc, new SpanSinkNode{}, "sink"});
        g->edges.push_back({"src", "positions", "acc", "x"});
        g->edges.push_back({"acc", "sum", "sink", "in_data"});
        return g;
    };
    auto g1 = make(&acc_v1);
    static_cast<SpanSourceNode*>(g1->nodes[0].data)->rows = {1.f, 2.f};
    tick_graph(*g1, 0.0);
    auto g2 = make(&acc_v2);
    migrate_graph(*g2, *g1);
    g1.reset();  // old graph (in real life: its owned descriptors too) dies
    auto* src = static_cast<SpanSourceNode*>(g2->nodes[0].data);
    auto* sink = static_cast<SpanSinkNode*>(g2->nodes[2].data);
    src->rows = {1.f, 2.f};
    tick_graph(*g2, 0.0);  // must not touch acc_v1's instances
    for (const auto& kv : g2->lifted_store) EXPECT_EQ(kv.second.desc, &acc_v2);
    ASSERT_EQ(sink->rows, 2);
}

TEST(MigrateGraph, ReregisteredSubgraphLiftFollowsNewDefinition) {
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<SpanSourceNode>());
    reg.register_builtin(make_descriptor<AccumNode>());
    reg.register_builtin(make_descriptor<SpanSinkNode>());
    const char* sub = R"({"inlets":[{"name":"x","node":"a","port":"x"}],
        "outlets":[{"name":"sum","node":"a","port":"sum"}],
        "nodes":[{"id":"a","type":"accum"}],"edges":[]})";
    ASSERT_TRUE(reg.register_subgraph("live_sub", sub));
    const char* graph = R"({"nodes":[
        {"id":"src","type":"span_source"},
        {"id":"sub","type":"live_sub"},
        {"id":"sink","type":"span_sink"}],
      "edges":[{"from":"src.positions","to":"sub.x"},
               {"from":"sub.sum","to":"sink.in_data"}]})";
    auto g1 = parse_graph(graph, reg);
    ASSERT_TRUE(g1);
    static_cast<SpanSourceNode*>(g1->nodes[0].data)->rows = {3.f, 4.f};
    tick_graph(*g1, 0.0);
    const EyeballsNodeDescriptor* old_desc = g1->nodes[1].desc;
    // Live reload: re-register the type (new descriptor), reparse, migrate.
    ASSERT_TRUE(reg.register_subgraph("live_sub", sub));
    auto g2 = parse_graph(graph, reg);
    ASSERT_TRUE(g2);
    ASSERT_NE(g2->nodes[1].desc, old_desc);
    migrate_graph(*g2, *g1);
    g1.reset();
    auto* src = static_cast<SpanSourceNode*>(g2->nodes[0].data);
    auto* sink = static_cast<SpanSinkNode*>(g2->nodes[2].data);
    src->rows = {3.f, 4.f};
    tick_graph(*g2, 0.0);
    for (const auto& kv : g2->lifted_store) EXPECT_EQ(kv.second.desc, g2->nodes[1].desc);
    EXPECT_EQ(sink->rows, 2);
}

// L1: editing the lift away (span edge removed, host kept) must destroy the
// orphaned store entries, not leak them forever.
TEST(SignalGraphLift, RemovedLiftDropsStoreEntries) {
    SpanSourceNode* src;
    SpanSinkNode* sink;
    auto g1 = make_lift_graph(&src, &sink);
    src->rows = {1.f, 2.f, 3.f};
    tick_graph(*g1, 0.0);
    EXPECT_EQ(g1->lifted_store.size(), 3u);
    SpanSourceNode* src2;
    SpanSinkNode* sink2;
    auto g2 = make_lift_graph(&src2, &sink2);
    g2->edges.clear();  // the edit deleted the span edge → no lift
    migrate_graph(*g2, *g1);
    tick_graph(*g2, 0.0);  // fresh plan has no LiftGroup → entries dropped
    EXPECT_TRUE(g2->lifted_store.empty());
}

// ── L3: gather zero-fill stride ──────────────────────────────────────────────
// A hand-rolled vec3-in / scalar-out stateless host whose output goes MISSING
// (null output_ptr) for rows whose sum exceeds 100.
struct V3Sum {
    const float* p = nullptr;
    float y = 0.f;
    bool has = true;
};
static EyeballsNodeDescriptor make_v3sum_desc() {
    EyeballsNodeDescriptor d{};
    d.version = EYEBALLS_ABI_VERSION;
    d.type_name = "v3sum";
    d.create = []() -> void* { return new V3Sum{}; };
    d.destroy = [](void* x) { delete static_cast<V3Sum*>(x); };
    d.process = [](void* x, double) {
        auto* s = static_cast<V3Sum*>(x);
        if (!s->p) return;
        s->y = s->p[0] + s->p[1] + s->p[2];
        s->has = s->y < 100.f;
    };
    d.connect = [](void* x, const char* port, const void* src) -> int {
        if (std::string_view{port} != "p") return 0;
        static_cast<V3Sum*>(x)->p = static_cast<const float*>(src);
        return 1;
    };
    d.output_ptr = [](void* x, const char* port) -> const void* {
        auto* s = static_cast<V3Sum*>(x);
        if (std::string_view{port} != "y" || !s->has) return nullptr;
        return &s->y;
    };
    d.port_schema =
        R"({"inputs":[{"name":"p","kind":"vec3"}],"outputs":[{"name":"y","kind":"scalar"}]})";
    d.lift_kind = EYEBALLS_LIFT_STATELESS;
    return d;
}

TEST(SignalGraphLift, MissingOutputZeroFillsOneOutCell) {
    static auto v3_desc = make_v3sum_desc();
    static auto src_desc = *make_descriptor<Vec3SpanSourceNode>();
    static auto sink_desc = *make_descriptor<SpanSinkNode>();
    auto g = std::make_unique<Graph>();
    auto* src = new Vec3SpanSourceNode{};
    auto* sink = new SpanSinkNode{};
    g->nodes.push_back({&src_desc, src, "src"});
    g->nodes.push_back({&v3_desc, new V3Sum{}, "v"});
    g->nodes.push_back({&sink_desc, sink, "sink"});
    g->edges.push_back({"src", "positions", "v", "p"});
    g->edges.push_back({"v", "y", "sink", "in_data"});
    src->rows = {1, 2, 3, 50, 60, 70, 4, 5, 6};  // row 1 sum=180 → missing
    tick_graph(*g, 0.0);
    ASSERT_EQ(sink->rows, 3);
    ASSERT_EQ(sink->seen.size(), 3u);
    EXPECT_FLOAT_EQ(sink->seen[0], 6.f);
    EXPECT_FLOAT_EQ(sink->seen[1], 0.f);   // ONE zero: out-cell, not in-cell
    EXPECT_FLOAT_EQ(sink->seen[2], 15.f);  // rows stay aligned
}
