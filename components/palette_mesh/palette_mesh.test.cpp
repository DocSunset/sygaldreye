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

// Editor-audit fix: palette_mesh.page consumes palette's page output — a
// flipped page draws that page's rows (20 types: page 0 has 15, page 1 has 5).
TEST(PaletteMesh, HonorsPageInput) {
    std::vector<std::string> types(20, "abc");
    PaletteMeshNode n;
    n.set_context({nullptr, nullptr, nullptr, &types});
    float page = 0.f;
    n.endpoints.page.src = &page;
    n(0.0);
    auto verts_p0 = n.endpoints.labels.value.geometry->vertices.size();
    page = 1.f;
    n(0.0);
    auto verts_p1 = n.endpoints.labels.value.geometry->vertices.size();
    EXPECT_LT(verts_p1, verts_p0);  // 5 rows + header < 15 rows + header
    EXPECT_GT(verts_p1, 0u);
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
