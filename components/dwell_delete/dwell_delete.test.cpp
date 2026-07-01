// Copyright 2026 Travis West
#include "dwell_delete.hpp"

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
}
void aim(DwellDeleteNode& n, const Eigen::Vector3f& target, bool grip, double t) {
    static Eigen::Vector3f p;
    static Eigen::Quaternionf q = Eigen::Quaternionf::Identity();
    static float g;
    p = target + Eigen::Vector3f{0, 0, 0.05f};
    g = grip ? 1.f : 0.f;
    n.endpoints.pos.src = &p;
    n.endpoints.rot.src = &q;
    n.endpoints.grip.src = &g;
    n(t);
}
}  // namespace

TEST(DwellDelete, GripDwellOverCardRemovesNode) {
    Graph g;
    make_graph(g);
    auto l = editor_layout::build_layout(g, {});
    Eigen::Vector3f body = l.cards[0].position;  // add0

    EventQueue<std::string> edits;
    DwellDeleteNode n;
    n.set_context({&g, &edits, nullptr, nullptr});

    aim(n, body, true, 0.0);  // start dwell (dt=0)
    aim(n, body, true, 0.6);  // +0.6 s
    aim(n, body, true, 1.2);  // crosses 1.0 s → delete

    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    EXPECT_EQ(ops[0], "{\"op\":\"remove_node\",\"id\":\"add0\"}");
}

TEST(DwellDelete, NoGripNoDelete) {
    Graph g;
    make_graph(g);
    auto l = editor_layout::build_layout(g, {});
    Eigen::Vector3f body = l.cards[0].position;
    EventQueue<std::string> edits;
    DwellDeleteNode n;
    n.set_context({&g, &edits, nullptr, nullptr});
    aim(n, body, false, 0.0);
    aim(n, body, false, 2.0);
    EXPECT_TRUE(edits.drain().empty());
}

TEST(DwellDelete, GripDwellOverEdgeRemovesEdge) {
    Graph g;
    make_graph(g);
    g.edges.push_back({"add0", "out", "mul0", "a"});
    auto l = editor_layout::build_layout(g, {});
    Eigen::Vector3f f, t;
    ASSERT_TRUE(editor_layout::edge_endpoints(l, g.edges[0], f, t));
    Eigen::Vector3f mid = (f + t) * 0.5f;
    // The midpoint must not be inside a card body (cards win). Verify.
    EventQueue<std::string> edits;
    DwellDeleteNode n;
    n.set_context({&g, &edits, nullptr, nullptr});
    aim(n, mid, true, 0.0);
    aim(n, mid, true, 1.2);
    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    EXPECT_EQ(ops[0], "{\"op\":\"remove_edge\",\"from\":\"add0.out\",\"to\":\"mul0.a\"}");
}
