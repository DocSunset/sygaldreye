// Copyright 2026 Travis West
#include "graph_source.hpp"

#include <gtest/gtest.h>

#include "eyeballs_node_abi.hpp"
#include "signal_graph_plan.hpp"

// A tiny stand-in graph: n nodes, ids only (graph_source reads ids).
static void fill_graph(Graph& g, int n) {
    static EyeballsNodeDescriptor d{};  // unused fields fine: only id is read
    for (int i = 0; i < n; ++i) g.nodes.push_back({&d, nullptr, "node_" + std::to_string(i)});
}

TEST(GraphSource, PublishesNodeCountAndShapes) {
    GraphSourceNode gs;
    Graph g;
    fill_graph(g, 3);
    gs.set_context(&g);
    gs(0.0);
    EXPECT_FLOAT_EQ(gs.endpoints.count.value, 3.f);
    EXPECT_EQ(gs.endpoints.keys.value.rows, 3);
    EXPECT_EQ(gs.endpoints.keys.value.cols, 1);
    EXPECT_EQ(gs.endpoints.positions.value.rows, 3);
    EXPECT_EQ(gs.endpoints.positions.value.cols, 3);
}

TEST(GraphSource, KeysAreStablePerIdAcrossReorder) {
    GraphSourceNode gs;
    Graph g;
    fill_graph(g, 2);
    gs.set_context(&g);
    gs(0.0);
    float k0 = gs.endpoints.keys.value.data[0];
    float k1 = gs.endpoints.keys.value.data[1];

    // Reorder: node_1, node_0. The key must FOLLOW the id, not the slot.
    std::swap(g.nodes[0], g.nodes[1]);
    gs(0.0);
    EXPECT_FLOAT_EQ(gs.endpoints.keys.value.data[0], k1);
    EXPECT_FLOAT_EQ(gs.endpoints.keys.value.data[1], k0);
    EXPECT_NE(k0, k1);
}

TEST(GraphSource, NoContextEmitsEmpty) {
    GraphSourceNode gs;
    gs(0.0);
    EXPECT_FLOAT_EQ(gs.endpoints.count.value, 0.f);
    EXPECT_EQ(gs.endpoints.keys.value.rows, 0);
}
