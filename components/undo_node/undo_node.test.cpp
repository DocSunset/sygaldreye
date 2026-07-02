// Copyright 2026 Travis West
#include "undo_node.hpp"

#include <gtest/gtest.h>

#include "eyeballs_node_abi.hpp"
#include "math_nodes.hpp"
#include "signal_graph_plan.hpp"

namespace {
void add_node(Graph& g, const char* id) {
    const EyeballsNodeDescriptor* add = make_descriptor<AddNode>();
    g.nodes.push_back({add, add->create(), id});
}
void tick(UndoNode& n, float thumb_x) {
    static float t;
    t = thumb_x;
    n.endpoints.thumb_x.src = &t;
    n(0.0);
}
}  // namespace

TEST(UndoNode, FlickRestoresPreviousGraph) {
    Graph g1;
    add_node(g1, "add0");
    std::string snap1 = serialize_graph(g1);

    EventQueue<std::string> edits;
    UndoNode n;
    n.set_context({&g1, &edits, nullptr, nullptr});
    tick(n, 0.f);  // baseline = snap1

    // Graph changes (a node added). Point the node at the new graph.
    Graph g2;
    add_node(g2, "add0");
    add_node(g2, "mul0");
    n.set_context({&g2, &edits, nullptr, nullptr});
    tick(n, 0.f);  // sees the change → previous_ = snap1

    tick(n, -0.9f);  // flick → restore snap1
    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    EXPECT_EQ(ops[0], snap1);
}

TEST(UndoNode, NoHistoryNoOp) {
    Graph g;
    add_node(g, "add0");
    EventQueue<std::string> edits;
    UndoNode n;
    n.set_context({&g, &edits, nullptr, nullptr});
    tick(n, 0.f);
    tick(n, -0.9f);  // nothing changed → nothing to undo
    EXPECT_TRUE(edits.drain().empty());
}

// §3 fix: idle frames must not re-serialize the whole graph — snapshots
// happen only when graph_gen (or the graph pointer) moves.
TEST(UndoNode, NoReserializeOnUnchangedGeneration) {
    Graph g;
    add_node(g, "add0");
    EventQueue<std::string> edits;
    UndoNode n;
    editor_layout::GestureContext ctx{&g, &edits, nullptr, nullptr};
    ctx.graph_gen = 1;
    n.set_context(ctx);
    tick(n, 0.f);
    EXPECT_EQ(n.snapshots(), 1u);
    tick(n, 0.f);
    tick(n, 0.f);
    EXPECT_EQ(n.snapshots(), 1u);  // idle: no serialize
    ctx.graph_gen = 2;             // a swap or param write happened
    n.set_context(ctx);
    tick(n, 0.f);
    EXPECT_EQ(n.snapshots(), 2u);
}
