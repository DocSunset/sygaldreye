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
    struct endpoints {
        normalled_in<float, fp(-1e6f), fp(1e6f), fp(0.f)> in;
        ::out<float> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.in.get() * 2.0f; }
};

// DrawNode: emits a draw call.
struct SgDrawNode {
    static consteval std::string_view name() { return "sg_draw"; }
    static bool drawn;
    struct endpoints { ::out<DrawFn> render; } endpoints;
    void operator()(double) {
        endpoints.render.value = [](const Eigen::Matrix4f&) { SgDrawNode::drawn = true; };
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

    EXPECT_NEAR(gain_node->endpoints.in.fallback, 0.75f, 1e-5f);
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
    static_cast<GainNode*>(clone->nodes[0].data)->endpoints.in.fallback = 9.0f;
    EXPECT_NEAR(static_cast<GainNode*>(src->nodes[0].data)->endpoints.in.fallback, 0.0f, 1e-6f);

    EXPECT_EQ(clone->inlets.size(), src->inlets.size());
    EXPECT_EQ(clone->outlets.size(), src->outlets.size());
}

// ── endpoints v6: outer wires forward through the subgraph boundary ──────────

struct V6ThruNode {
    static consteval std::string_view name() { return "v6_thru"; }
    struct endpoints {
        ::in<float, fp(-1.f)> in;
        ::out<float> out;
    } endpoints;
    void operator()(double) { endpoints.out.value = endpoints.in.get() * 2.f; }
};

TEST(SubgraphNode, V6WiresForwardToInnerStorage) {
    ComponentRegistry reg;
    static auto desc = make_descriptor<V6ThruNode>();
    reg.register_builtin(desc);
    auto inner = parse_graph(R"({
        "nodes":[{"id":"t","type":"v6_thru","params":{}}],
        "edges":[],
        "inlets":[{"name":"gain","node":"t","port":"in"}],
        "outlets":[{"name":"level","node":"t","port":"out"}]
    })", reg);
    ASSERT_NE(inner, nullptr);
    auto* thru = static_cast<V6ThruNode*>(inner->nodes[0].data);

    SubgraphNode sn(std::move(inner));
    // outlet_ptr returns the INNER node's storage address
    EXPECT_EQ(sn.outlet_ptr("level"),
              static_cast<const void*>(&thru->endpoints.out.value));
    // connect_inlet points the inner input at an outer producer
    float outer = 3.f;
    EXPECT_EQ(sn.connect_inlet("gain", &outer), 1);
    sn(0.0);
    EXPECT_FLOAT_EQ(thru->endpoints.out.value, 6.f);
    // disconnect: inner input falls back to its compile-time default
    EXPECT_EQ(sn.connect_inlet("gain", nullptr), 1);
    sn(0.1);
    EXPECT_FLOAT_EQ(thru->endpoints.out.value, -2.f);
}
