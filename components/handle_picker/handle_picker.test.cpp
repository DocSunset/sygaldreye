// Copyright 2026 Travis West
#include "handle_picker.hpp"

#include <gtest/gtest.h>

#include "eyeballs_node_abi.hpp"
#include "math_nodes.hpp"
#include "signal_graph_plan.hpp"

namespace {
void make_graph(Graph& g) {
    const EyeballsNodeDescriptor* add = make_descriptor<AddNode>();
    g.nodes.push_back({add, add->create(), "add0"});
}
void aim(HandlePickerNode& n, const Eigen::Vector3f& target) {
    static Eigen::Vector3f p;
    static Eigen::Quaternionf q = Eigen::Quaternionf::Identity();
    p = target + Eigen::Vector3f{0, 0, 0.05f};
    n.endpoints.pos.src = &p;
    n.endpoints.rot.src = &q;
    n(0.0);
}
}  // namespace

TEST(HandlePicker, NearInputHandleLabelsPort) {
    Graph g;
    make_graph(g);
    auto l = editor_layout::build_layout(g, {});
    HandlePickerNode n;
    n.set_context({&g, nullptr, nullptr, nullptr});
    aim(n, l.cards[0].inputs[0].world_pos);  // "a" input handle
    EXPECT_EQ(n.endpoints.label.value, std::string("a ◂"));
}

TEST(HandlePicker, NearOutputHandleLabelsPort) {
    Graph g;
    make_graph(g);
    auto l = editor_layout::build_layout(g, {});
    HandlePickerNode n;
    n.set_context({&g, nullptr, nullptr, nullptr});
    aim(n, l.cards[0].outputs[0].world_pos);  // "out" output handle
    EXPECT_EQ(n.endpoints.label.value, std::string("▸ out"));
}

TEST(HandlePicker, FarAwayNoLabel) {
    Graph g;
    make_graph(g);
    HandlePickerNode n;
    n.set_context({&g, nullptr, nullptr, nullptr});
    aim(n, {10.f, 10.f, 10.f});
    EXPECT_TRUE(n.endpoints.label.value.empty());
}
