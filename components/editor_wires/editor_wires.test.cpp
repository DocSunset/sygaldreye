// Copyright 2026 Travis West
#include "editor_wires.hpp"

#include <gtest/gtest.h>

#include "eyeballs_node_abi.hpp"
#include "math_nodes.hpp"
#include "signal_graph_plan.hpp"

namespace {
void make_graph(Graph& g) {
    const EyeballsNodeDescriptor* add = make_descriptor<AddNode>();
    const EyeballsNodeDescriptor* mul = make_descriptor<MulNode>();
    g.nodes.push_back({add, add->create(), "add0"});
    g.nodes.push_back({mul, mul->create(), "mul0"});
    g.edges.push_back({"add0", "out", "mul0", "a"});
}
}  // namespace

TEST(EditorWires, OneRowPerEdge) {
    Graph g;
    make_graph(g);
    EditorWiresNode n;
    n.set_context({&g, nullptr, nullptr, nullptr});
    n(0.0);
    EXPECT_EQ(n.endpoints.wires.value.rows, 1);
    EXPECT_EQ(n.endpoints.wires.value.cols, 10);
    // Endpoints match the layout's handle world positions.
    auto l = editor_layout::build_layout(g, {});
    Eigen::Vector3f f, t;
    ASSERT_TRUE(editor_layout::edge_endpoints(l, g.edges[0], f, t));
    const float* r = n.endpoints.wires.value.data;
    EXPECT_FLOAT_EQ(r[0], f.x());
    EXPECT_FLOAT_EQ(r[3], t.x());
}

TEST(EditorWires, NoGraphEmpty) {
    EditorWiresNode n;
    n(0.0);
    EXPECT_EQ(n.endpoints.wires.value.rows, 0);
}
