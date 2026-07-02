// Copyright 2026 Travis West
#include "wire_drag.hpp"

#include <gtest/gtest.h>

#include "eyeballs_node_abi.hpp"
#include "math_nodes.hpp"
#include "signal_graph_plan.hpp"

using editor_layout::GestureContext;
using editor_layout::Layout;

namespace {
void make_graph(Graph& g) {
    const EyeballsNodeDescriptor* add = make_descriptor<AddNode>();
    const EyeballsNodeDescriptor* mul = make_descriptor<MulNode>();
    g.nodes.push_back({add, add->create(), "add0"});
    g.nodes.push_back({mul, mul->create(), "mul0"});
}

// A pose whose tip (5 cm along -z) lands exactly at `target`: place the pose
// 5 cm in +z so identity-forward reaches it.
void aim_at(WireDragNode& n, const Eigen::Vector3f& target, bool grip) {
    n.endpoints.pos.src = nullptr;
    static Eigen::Vector3f p;
    static Eigen::Quaternionf q = Eigen::Quaternionf::Identity();
    static float g;
    p = target + Eigen::Vector3f{0, 0, 0.05f};
    g = grip ? 1.f : 0.f;
    n.endpoints.pos.src = &p;
    n.endpoints.rot.src = &q;
    n.endpoints.grip.src = &g;
    n(0.0);
}
}  // namespace

TEST(WireDrag, GripConnectEmitsAddEdge) {
    Graph g;
    make_graph(g);
    auto l = editor_layout::build_layout(g, {});
    Eigen::Vector3f out_h = l.cards[0].outputs[0].world_pos;  // add0.out
    Eigen::Vector3f in_h = l.cards[1].inputs[0].world_pos;    // mul0.a

    EventQueue<std::string> edits;
    editor_layout::PosOverrides ovr;
    WireDragNode n;
    n.set_context({&g, &edits, &ovr});

    aim_at(n, out_h, false);  // idle, not gripping
    aim_at(n, out_h, true);   // grip-down at the output handle → grab
    aim_at(n, in_h, true);    // move to the input handle, still gripping
    aim_at(n, in_h, false);   // grip-up → connect

    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    EXPECT_EQ(ops[0], "{\"op\":\"add_edge\",\"from\":\"add0.out\",\"to\":\"mul0.a\"}");
}

TEST(WireDrag, GripCardBodyMovesCard) {
    Graph g;
    make_graph(g);
    auto l = editor_layout::build_layout(g, {});
    Eigen::Vector3f body = l.cards[0].position;  // add0 body, away from handles

    EventQueue<std::string> edits;
    editor_layout::PosOverrides ovr;
    WireDragNode n;
    n.set_context({&g, &edits, &ovr});

    aim_at(n, body, false);
    aim_at(n, body, true);  // grab body
    Eigen::Vector3f moved = body + Eigen::Vector3f{0.3f, 0.1f, 0.f};
    aim_at(n, moved, true);  // drag

    ASSERT_TRUE(ovr.count("add0"));
    EXPECT_NEAR(ovr["add0"].x(), moved.x(), 1e-4f);
    EXPECT_TRUE(edits.drain().empty());  // a move is not an edit op
}

// §3 fix: a card drag bumps overrides_gen so the shared layout cache stays
// live during the drag.
TEST(WireDrag, CardMoveBumpsOverridesGeneration) {
    Graph g;
    make_graph(g);
    auto l = editor_layout::build_layout(g, {});
    Eigen::Vector3f body = l.cards[0].position;

    EventQueue<std::string> edits;
    editor_layout::PosOverrides ovr;
    std::uint64_t ogen = 0;
    GestureContext ctx{&g, &edits, &ovr};
    ctx.overrides_gen = &ogen;
    WireDragNode n;
    n.set_context(ctx);

    aim_at(n, body, false);
    aim_at(n, body, true);                                    // grab
    aim_at(n, body + Eigen::Vector3f{0.2f, 0, 0}, true);      // drag
    EXPECT_GT(ogen, 0u);
}
