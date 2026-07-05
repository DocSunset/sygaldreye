// Copyright 2026 Travis West
#include "palette.hpp"

#include <gtest/gtest.h>

#include "signal_graph_plan.hpp"

namespace {
void poke(PaletteNode& n, const Eigen::Vector3f& target, bool trig) {
    static Eigen::Vector3f p;
    static Eigen::Quaternionf q = Eigen::Quaternionf::Identity();
    static float t;
    p = target + Eigen::Vector3f{0, 0, 0.05f};
    t = trig ? 1.f : 0.f;
    n.endpoints.pos.src = &p;
    n.endpoints.rot.src = &q;
    n.endpoints.trigger.src = &t;
    n(0.0);
}
// World y of palette row r (r=0 header, r>=1 type rows). Mirrors panel layout.
float row_y(int r) {
    float v = (float(r) + 0.5f) / float(PaletteNode::kRows + 1);
    return PaletteNode::panel_pos().y() + PaletteNode::panel_h() * (0.5f - v);
}
}  // namespace

TEST(Palette, PokeOnTypeRowAddsNode) {
    Graph g;
    std::vector<std::string> types = {"add", "mul", "const"};
    EventQueue<std::string> edits;
    PaletteNode n;
    n.set_context({&g, &edits, nullptr, &types});

    Eigen::Vector3f r1 = PaletteNode::panel_pos();
    r1.y() = row_y(1);  // first type row → types[0] = "add"
    poke(n, r1, false);
    poke(n, r1, true);  // trigger rising → add_node

    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    EXPECT_EQ(ops[0], "{\"op\":\"add_node\",\"type\":\"add\"}");
}

TEST(Palette, PokeHeaderFlipsPage) {
    Graph g;
    std::vector<std::string> types(20, "x");  // 20 types → 2 pages
    EventQueue<std::string> edits;
    PaletteNode n;
    n.set_context({&g, &edits, nullptr, &types});

    Eigen::Vector3f r0 = PaletteNode::panel_pos();
    r0.y() = row_y(0);  // header
    EXPECT_EQ(n.page(), 0);
    poke(n, r0, true);
    EXPECT_EQ(n.page(), 1);
    EXPECT_TRUE(edits.drain().empty());  // a page flip is not an add
}
