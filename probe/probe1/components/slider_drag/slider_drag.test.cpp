// Copyright 2026 Travis West
#include "slider_drag.hpp"

#include <gtest/gtest.h>

#include "eyeballs_node_abi.hpp"
#include "math_nodes.hpp"
#include "signal_graph_plan.hpp"

using editor_layout::Layout;

namespace {
void make_graph(Graph& g) {
    const EyeballsNodeDescriptor* add = make_descriptor<AddNode>();
    g.nodes.push_back({add, add->create(), "add0"});
}
void aim(SliderDragNode& n, const Eigen::Vector3f& target, bool trig) {
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
}  // namespace

TEST(SliderDrag, TipOverTrackEmitsSetParam) {
    Graph g;
    make_graph(g);
    auto l = editor_layout::build_layout(g, {});
    // add0's first slider (port "a"); push the tip to the right end → value=max.
    const auto& sl = l.cards[0].sliders[0];
    Eigen::Vector3f right_end = sl.world_pos + Eigen::Vector3f{sl.width * 0.49f, 0, 0};

    EventQueue<std::string> edits;
    SliderDragNode n;
    n.set_context({&g, &edits, nullptr, nullptr});
    aim(n, right_end, true);

    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    // "a" port range is [-1000,1000]; near-max ≈ +980.
    EXPECT_NE(ops[0].find("\"op\":\"set_param\""), std::string::npos);
    EXPECT_NE(ops[0].find("\"id\":\"add0\""), std::string::npos);
    EXPECT_NE(ops[0].find("\"port\":\"a\""), std::string::npos);
}

TEST(SliderDrag, NoTriggerNoOp) {
    Graph g;
    make_graph(g);
    auto l = editor_layout::build_layout(g, {});
    EventQueue<std::string> edits;
    SliderDragNode n;
    n.set_context({&g, &edits, nullptr, nullptr});
    aim(n, l.cards[0].sliders[0].world_pos, false);
    EXPECT_TRUE(edits.drain().empty());
}
