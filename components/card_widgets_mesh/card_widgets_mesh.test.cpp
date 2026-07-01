// Copyright 2026 Travis West
#include "card_widgets_mesh.hpp"

#include <gtest/gtest.h>

#include "eyeballs_node_abi.hpp"
#include "math_nodes.hpp"
#include "signal_graph_plan.hpp"
#include "tri_mesh.hpp"

namespace {
void make_graph(Graph& g) {
    const EyeballsNodeDescriptor* add = make_descriptor<AddNode>();  // a,b in; out
    g.nodes.push_back({add, add->create(), "add0"});
}
}  // namespace

TEST(CardWidgetsMesh, EmitsHandleAndSliderQuads) {
    Graph g;
    make_graph(g);
    CardWidgetsMeshNode n;
    editor_layout::PosOverrides ovr;
    n.set_context({&g, nullptr, &ovr, nullptr});
    n(0.0);

    MeshPtr m = n.endpoints.mesh.value;
    ASSERT_TRUE(m);
    // add0: 2 input handles + 1 output handle = 3 handle quads; 2 sliders ⇒
    // 2 tracks + 2 thumbs = 4 quads. 7 quads × 4 verts = 28 verts, 42 indices.
    EXPECT_EQ(m->vertices.size(), 28u);
    EXPECT_EQ(m->indices.size(), 42u);
}

TEST(CardWidgetsMesh, NoGraphEmitsNull) {
    CardWidgetsMeshNode n;
    n(0.0);
    EXPECT_FALSE(n.endpoints.mesh.value);
}
