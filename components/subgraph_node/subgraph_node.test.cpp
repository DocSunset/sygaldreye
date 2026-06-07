// Copyright 2025 Travis West
#include "subgraph_node.hpp"
#include "signal_graph.hpp"
#include "component_registry.hpp"
#include "eyeballs_node_abi.hpp"
#include "sygaldry_endpoints.hpp"
#include <gtest/gtest.h>

// GainNode: scalar input "in", scalar output "out" = in * 2.
struct GainNode {
    static consteval std::string_view name() { return "gain"; }
    struct inputs  { port<"in",  float> in;  } inputs;
    struct outputs { port<"out", float> out; } outputs;
    void operator()(double) { outputs.out.value = inputs.in.value * 2.0f; }
};

// DrawNode: emits a draw call.
struct SgDrawNode {
    static consteval std::string_view name() { return "sg_draw"; }
    static bool drawn;
    struct inputs {} inputs;
    struct outputs { port<"render", DrawFn> render; } outputs;
    void operator()(double) {
        outputs.render.value = [](const Eigen::Matrix4f&) { SgDrawNode::drawn = true; };
    }
};
bool SgDrawNode::drawn = false;

static std::unique_ptr<Graph> make_gain_graph(ComponentRegistry& reg) {
    static auto desc = make_descriptor<GainNode>();
    reg.register_builtin(desc);
    const char* json = R"({
        "nodes":[{"id":"g","type":"gain","params":{}}],
        "edges":[],
        "inlets":[{"name":"gain","node":"g","port":"in"}],
        "outlets":[{"name":"level","node":"g","port":"out"}]
    })";
    return parse_graph(json, reg);
}

TEST(SubgraphNode, InletPropagation) {
    ComponentRegistry reg;
    auto inner = make_gain_graph(reg);
    ASSERT_NE(inner, nullptr);
    auto* gain_node = static_cast<GainNode*>(inner->nodes[0].data);

    SubgraphNode sn(std::move(inner));
    sn.cache_inlet("gain", PortValue{0.75});
    sn(0.0);

    EXPECT_NEAR(gain_node->inputs.in.value, 0.75f, 1e-5f);
}

TEST(SubgraphNode, OutletEmission) {
    ComponentRegistry reg;
    auto inner = make_gain_graph(reg);
    ASSERT_NE(inner, nullptr);

    SubgraphNode sn(std::move(inner));
    sn.cache_inlet("gain", PortValue{0.5});
    sn(0.0);

    std::unordered_map<std::string, PortValue> store;
    EyeballsOutputCtx ctx{};
    ctx.store   = &store;
    ctx.node_id = "sub";
    ctx.emit_scalar = [](void* s, const char* nid, const char* port, double v) {
        auto& m = *static_cast<std::unordered_map<std::string, PortValue>*>(s);
        m[std::string(nid) + "." + port] = v;
    };
    sn.push_outlets(&ctx);

    auto it = store.find("sub.level");
    ASSERT_NE(it, store.end());
    EXPECT_NEAR(std::get<double>(it->second), 1.0, 1e-6);  // 0.5 * 2
}

TEST(SubgraphNode, DrawCallForwarding) {
    ComponentRegistry reg;
    static auto desc = make_descriptor<SgDrawNode>();
    reg.register_builtin(desc);
    const char* json = R"({"nodes":[{"id":"d","type":"sg_draw","params":{}}],"edges":[]})";
    auto inner = parse_graph(json, reg);
    ASSERT_NE(inner, nullptr);

    SubgraphNode sn(std::move(inner));
    sn(0.0);

    std::vector<DrawFn> calls;
    DrawCallCtx ctx{"sub", &calls};
    sn.push_draw_calls_to(&ctx);
    ASSERT_EQ(calls.size(), 1u);

    SgDrawNode::drawn = false;
    calls[0](Eigen::Matrix4f::Identity());
    EXPECT_TRUE(SgDrawNode::drawn);
}

TEST(SubgraphNode, CloneIndependence) {
    ComponentRegistry reg;
    auto src = make_gain_graph(reg);
    ASSERT_NE(src, nullptr);

    auto clone = clone_graph(*src);
    ASSERT_EQ(clone->nodes.size(), 1u);
    ASSERT_NE(clone->nodes[0].data, src->nodes[0].data);  // distinct instances

    // Mutate the clone's node; original must be unaffected.
    static_cast<GainNode*>(clone->nodes[0].data)->inputs.in.value = 9.0f;
    EXPECT_NEAR(static_cast<GainNode*>(src->nodes[0].data)->inputs.in.value, 0.0f, 1e-6f);

    EXPECT_EQ(clone->inlets.size(), src->inlets.size());
    EXPECT_EQ(clone->outlets.size(), src->outlets.size());
}
