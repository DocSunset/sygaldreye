// Copyright 2026 Travis West
#include "edit_sink.hpp"

#include <gtest/gtest.h>

#include <string_view>

#include "component_registry.hpp"
#include "eyeballs_node_abi.hpp"
#include "signal_graph.hpp"
#include "signal_graph_plan.hpp"

// A trivial registrable node (scalar in/out) so apply_edit_op round-trips
// through a real registry + parse_graph.
struct PassNode {
    static consteval std::string_view name() { return "pass"; }
    struct endpoints {
        normalled_in<float, fp(0.f), fp(10.f), fp(1.f)> g;
        out<float> y;
    } endpoints;
    void operator()(double) { endpoints.y.value = endpoints.g.get(); }
};

static void make_reg(ComponentRegistry& r) { r.register_builtin(make_descriptor<PassNode>()); }

TEST(EditSink, PushesDistinctCommandsOnce) {
    EventQueue<std::string> q;
    EditSinkNode s;
    s.set_context(&q);
    s.endpoints.command.fallback = "{\"op\":\"add_node\",\"type\":\"pass\"}";
    s(0.0);
    s(0.0);  // same command held → no second push
    EXPECT_EQ(q.drain().size(), 1u);
}

TEST(EditOp, AddNodeAddEdgeGrowsGraph) {
    ComponentRegistry reg;
    make_reg(reg);
    auto g = parse_graph(R"({"nodes":[{"id":"a","type":"pass"}],"edges":[]})", reg);
    ASSERT_TRUE(g);
    std::string j = apply_edit_op("{\"op\":\"add_node\",\"type\":\"pass\",\"id\":\"b\"}", *g);
    auto g2 = parse_graph(j, reg);
    ASSERT_TRUE(g2);
    EXPECT_EQ(g2->nodes.size(), 2u);
    std::string j2 = apply_edit_op("{\"op\":\"add_edge\",\"from\":\"a.y\",\"to\":\"b.g\"}", *g2);
    auto g3 = parse_graph(j2, reg);
    ASSERT_TRUE(g3);
    EXPECT_EQ(g3->edges.size(), 1u);
}

TEST(EditOp, RemoveNodeDropsEdges) {
    ComponentRegistry reg;
    make_reg(reg);
    auto g = parse_graph(
        R"({"nodes":[{"id":"a","type":"pass"},{"id":"b","type":"pass"}],
                            "edges":[{"from":"a.y","to":"b.g"}]})",
        reg);
    ASSERT_TRUE(g);
    std::string j = apply_edit_op("{\"op\":\"remove_node\",\"id\":\"a\"}", *g);
    auto g2 = parse_graph(j, reg);
    ASSERT_TRUE(g2);
    EXPECT_EQ(g2->nodes.size(), 1u);
    EXPECT_EQ(g2->edges.size(), 0u);  // the dangling edge went too
}

TEST(EditOp, RemoveEdgeKeepsNodes) {
    ComponentRegistry reg;
    make_reg(reg);
    auto g = parse_graph(
        R"({"nodes":[{"id":"a","type":"pass"},{"id":"b","type":"pass"}],
                            "edges":[{"from":"a.y","to":"b.g"}]})",
        reg);
    ASSERT_TRUE(g);
    std::string j = apply_edit_op("{\"op\":\"remove_edge\",\"from\":\"a.y\",\"to\":\"b.g\"}", *g);
    auto g2 = parse_graph(j, reg);
    ASSERT_TRUE(g2);
    EXPECT_EQ(g2->nodes.size(), 2u);
    EXPECT_EQ(g2->edges.size(), 0u);
}

TEST(EditOp, SetParamPersistsThroughParse) {
    ComponentRegistry reg;
    make_reg(reg);
    auto g = parse_graph(R"({"nodes":[{"id":"a","type":"pass"}],"edges":[]})", reg);
    ASSERT_TRUE(g);
    std::string j =
        apply_edit_op("{\"op\":\"set_param\",\"id\":\"a\",\"port\":\"g\",\"value\":7.5}", *g);
    auto g2 = parse_graph(j, reg);
    ASSERT_TRUE(g2);
    tick_graph(*g2, 0.0);
    auto v = read_output(g2->nodes[0], "y", "scalar");
    ASSERT_TRUE(v);
    EXPECT_FLOAT_EQ(float(std::get<double>(*v)), 7.5f);
}

TEST(EditOp, WholeGraphPassesThrough) {
    ComponentRegistry reg;
    make_reg(reg);
    auto g = parse_graph(R"({"nodes":[{"id":"a","type":"pass"}],"edges":[]})", reg);
    ASSERT_TRUE(g);
    std::string whole = R"({"nodes":[{"id":"x","type":"pass"}],"edges":[]})";
    EXPECT_EQ(apply_edit_op(whole, *g), whole);
}
