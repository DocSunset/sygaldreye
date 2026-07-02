// Copyright 2026 Travis West
#include "editor_layout.hpp"

#include <gtest/gtest.h>

#include "eyeballs_node_abi.hpp"
#include "math_nodes.hpp"
#include "signal_graph_plan.hpp"

using namespace editor_layout;

namespace {
// One graph of real nodes: add (a,b → out), mul (a,b → out). (Graph is
// move-only with a user dtor, so fill in place rather than return by value.)
void make_graph(Graph& g) {
    const EyeballsNodeDescriptor* add = make_descriptor<AddNode>();
    const EyeballsNodeDescriptor* mul = make_descriptor<MulNode>();
    g.nodes.push_back({add, add->create(), "add0"});
    g.nodes.push_back({mul, mul->create(), "mul0"});
}
}  // namespace

TEST(EditorLayout, OneCardPerNodeWithHandlesAndSliders) {
    Graph g;
    make_graph(g);
    auto l = build_layout(g, {});
    ASSERT_EQ(l.cards.size(), 2u);
    // add: a,b inputs (2 handles + 2 sliders), out output (1 handle).
    EXPECT_EQ(l.cards[0].node_id, "add0");
    EXPECT_EQ(l.cards[0].inputs.size(), 2u);
    EXPECT_EQ(l.cards[0].outputs.size(), 1u);
    EXPECT_EQ(l.cards[0].sliders.size(), 2u);
    // Input handles sit left of the card, outputs right.
    EXPECT_LT(l.cards[0].inputs[0].world_pos.x(), l.cards[0].position.x());
    EXPECT_GT(l.cards[0].outputs[0].world_pos.x(), l.cards[0].position.x());
    // Row pitch 0.018 m between consecutive input handles.
    float dy = l.cards[0].inputs[0].world_pos.y() - l.cards[0].inputs[1].world_pos.y();
    EXPECT_NEAR(dy, kPortRowH, 1e-5f);
}

TEST(EditorLayout, OverrideMovesCard) {
    Graph g;
    make_graph(g);
    std::unordered_map<std::string, Eigen::Vector3f> ovr;
    ovr["mul0"] = {2.f, 3.f, -1.f};
    auto l = build_layout(g, ovr);
    EXPECT_FLOAT_EQ(l.cards[1].position.x(), 2.f);
    EXPECT_FLOAT_EQ(l.cards[1].position.y(), 3.f);
    // Handles follow the moved card.
    EXPECT_NEAR(l.cards[1].inputs[0].world_pos.x(), 2.f - kCardW * 0.5f - kHandleOffX, 1e-5f);
}

TEST(EditorLayout, EdgeEndpointsResolve) {
    Graph g;
    make_graph(g);
    auto l = build_layout(g, {});
    Edge e{"add0", "out", "mul0", "a"};
    Eigen::Vector3f from, to;
    ASSERT_TRUE(edge_endpoints(l, e, from, to));
    // from = add0 output handle, to = mul0 input handle.
    EXPECT_GT(from.x(), l.cards[0].position.x());
    EXPECT_LT(to.x(), l.cards[1].position.x());
}

TEST(EditorLayout, KeyMatchesGraphSourceHash) {
    // The same FNV-1a graph_source publishes (24-bit).
    EXPECT_NE(id_key("add0"), id_key("mul0"));
    EXPECT_FLOAT_EQ(id_key("add0"), id_key("add0"));
}

// The shared cache rebuilds only when (graph, graph_gen, overrides_gen)
// moves — 7 nodes per frame hit the same build.
TEST(EditorLayout, CachedLayoutMemoizesOnGenerations) {
    Graph g;
    make_graph(g);
    LayoutCache cache;
    std::uint64_t ogen = 0;
    GestureContext ctx{&g, nullptr, nullptr, nullptr, nullptr, &cache, 0, &ogen};
    const Layout* a = &cached_layout(ctx);
    const Layout* b = &cached_layout(ctx);
    EXPECT_EQ(a, b);
    EXPECT_EQ(cache.builds, 1u);  // hit on identical gens
    ctx.graph_gen = 1;            // swap or in-place param write
    cached_layout(ctx);
    EXPECT_EQ(cache.builds, 2u);
    ++ogen;  // card drag
    cached_layout(ctx);
    EXPECT_EQ(cache.builds, 3u);
    cached_layout(ctx);
    EXPECT_EQ(cache.builds, 3u);  // steady state: no rebuild
}

TEST(EditorLayout, CachedLayoutWithoutCacheRebuildsEveryCall) {
    Graph g;
    make_graph(g);
    GestureContext ctx{&g, nullptr, nullptr, nullptr};
    EXPECT_EQ(cached_layout(ctx).cards.size(), 2u);  // no cache: plain build
    EXPECT_EQ(cached_layout(ctx).cards.size(), 2u);
}

TEST(EditorLayout, JsonEscapeQuotesAndBackslashes) {
    EXPECT_EQ(json_escape("plain"), "plain");
    EXPECT_EQ(json_escape("a\"b\\c"), "a\\\"b\\\\c");
}
