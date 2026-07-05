// Copyright 2026 Travis West
#include "palette_mesh.hpp"

#include <gtest/gtest.h>

#include "signal_graph_plan.hpp"

TEST(PaletteMesh, EmitsPanelAndLabels) {
    std::vector<std::string> types = {"add", "mul", "const"};
    PaletteMeshNode n;
    n.set_context({nullptr, nullptr, nullptr, &types});
    n(0.0);
    // Panel: one quad (4 verts, 6 indices).
    ASSERT_TRUE(n.endpoints.panel.value.geometry);
    EXPECT_EQ(n.endpoints.panel.value.geometry->vertices.size(), 4u);
    // Labels: header + 3 type rows ⇒ glyphs present + atlas surface.
    ASSERT_TRUE(n.endpoints.labels.value.geometry);
    EXPECT_GT(n.endpoints.labels.value.geometry->vertices.size(), 0u);
    EXPECT_FALSE(n.endpoints.labels_surface.value.images.empty());
}

TEST(PaletteMesh, NoTypesStillDrawsHeader) {
    PaletteMeshNode n;
    n.set_context({nullptr, nullptr, nullptr, nullptr});
    n(0.0);
    ASSERT_TRUE(n.endpoints.panel.value.geometry);
    EXPECT_EQ(n.endpoints.panel.value.geometry->vertices.size(), 4u);
    // The "more... 1/1" header still lays out glyphs.
    EXPECT_GT(n.endpoints.labels.value.geometry->vertices.size(), 0u);
}
