// Copyright 2026 Travis West
#include "card_labels_mesh.hpp"

#include <gtest/gtest.h>

#include "eyeballs_node_abi.hpp"
#include "math_nodes.hpp"
#include "signal_graph_plan.hpp"
#include "tri_mesh.hpp"

namespace {
void make_graph(Graph& g) {
    const EyeballsNodeDescriptor* add = make_descriptor<AddNode>();
    const EyeballsNodeDescriptor* mul = make_descriptor<MulNode>();
    g.nodes.push_back({add, add->create(), "add0"});
    g.nodes.push_back({mul, mul->create(), "mul0"});
}
}  // namespace

TEST(CardLabelsMesh, EmitsGlyphsForEveryCard) {
    Graph g;
    make_graph(g);
    CardLabelsMeshNode n;
    n.set_context({&g, nullptr, nullptr, nullptr});
    n(0.0);
    // Two non-empty ids ⇒ glyph quads emitted; atlas surface present.
    ASSERT_TRUE(n.endpoints.mesh.value.geometry);
    EXPECT_GT(n.endpoints.mesh.value.geometry->vertices.size(), 0u);
    EXPECT_TRUE(n.endpoints.surface.value.shader);
    EXPECT_FALSE(n.endpoints.surface.value.images.empty());  // atlas bound
}

TEST(CardLabelsMesh, NoGraphEmptyGeometryButValidSurface) {
    CardLabelsMeshNode n;
    n(0.0);
    ASSERT_TRUE(n.endpoints.mesh.value.geometry);
    EXPECT_EQ(n.endpoints.mesh.value.geometry->vertices.size(), 0u);
    EXPECT_TRUE(n.endpoints.surface.value.shader);
}
