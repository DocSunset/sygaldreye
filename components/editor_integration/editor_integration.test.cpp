// Copyright 2026 Travis West
// Editor gesture → edit-op → graph round-trip (vr_editor_decomposition.md
// verification): each gesture node, fed controller poses against a real graph,
// emits the SAME structured edit the monolith produced; apply_edit_op +
// parse_graph then realise it. No GL — pure data flow.
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "component_registry.hpp"
#include "dwell_delete.hpp"
#include "editor_layout.hpp"
#include "eyeballs_node_abi.hpp"
#include "math_nodes.hpp"
#include "palette.hpp"
#include "peer_core.hpp"
#include "signal_graph.hpp"
#include "signal_graph_plan.hpp"
#include "slider_drag.hpp"
#include "subgraph_node.hpp"
#include "wire_drag.hpp"

// PeerCore's screenshot writer lives in exactly one shell TU; the test is
// the shell here (never called).
extern "C" int stbi_write_png(const char*, int, int, int, const void*, int) { return 1; }

namespace {
void make_registry(ComponentRegistry& r) {
    r.register_builtin(make_descriptor<AddNode>());
    r.register_builtin(make_descriptor<MulNode>());
}
void two_nodes(Graph& g, const ComponentRegistry& r) {
    const EyeballsNodeDescriptor* add = r.find("add");
    const EyeballsNodeDescriptor* mul = r.find("mul");
    g.nodes.push_back({add, add->create(), "add0"});
    g.nodes.push_back({mul, mul->create(), "mul0"});
}
// Drive a gesture node's pos/rot inputs so its tip lands at `target`.
template <typename N>
void aim(N& n, const Eigen::Vector3f& target, float btn, const float** keep) {
    static Eigen::Vector3f p;
    static Eigen::Quaternionf q = Eigen::Quaternionf::Identity();
    static float b;
    p = target + Eigen::Vector3f{0, 0, 0.05f};
    b = btn;
    n.endpoints.pos.src = &p;
    n.endpoints.rot.src = &q;
    *keep = &b;
}
}  // namespace

// wire_drag connect → add_edge op → parse_graph realises the new edge.
TEST(EditorIntegration, WireDragConnectRoundTrips) {
    ComponentRegistry reg;
    make_registry(reg);
    Graph g;
    two_nodes(g, reg);
    auto l = editor_layout::build_layout(g, {});

    EventQueue<std::string> edits;
    editor_layout::PosOverrides ovr;
    WireDragNode n;
    n.set_context({&g, &edits, &ovr, nullptr});
    const float* gp = nullptr;
    Eigen::Vector3f out_h = l.cards[0].outputs[0].world_pos;
    Eigen::Vector3f in_h = l.cards[1].inputs[0].world_pos;
    aim(n, out_h, 0.f, &gp);
    n.endpoints.grip.src = gp;
    n(0.0);
    aim(n, out_h, 1.f, &gp);
    n.endpoints.grip.src = gp;
    n(0.0);  // grip down
    aim(n, in_h, 1.f, &gp);
    n.endpoints.grip.src = gp;
    n(0.0);
    aim(n, in_h, 0.f, &gp);
    n.endpoints.grip.src = gp;
    n(0.0);  // grip up → connect

    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    std::string json = apply_edit_op(ops[0], g);
    auto g2 = parse_graph(json, reg);
    ASSERT_TRUE(g2);
    ASSERT_EQ(g2->edges.size(), 1u);
    EXPECT_EQ(g2->edges[0].from_node, "add0");
    EXPECT_EQ(g2->edges[0].from_port, "out");
    EXPECT_EQ(g2->edges[0].to_node, "mul0");
    EXPECT_EQ(g2->edges[0].to_port, "a");
}

// slider_drag → set_param op → the node's param changes.
TEST(EditorIntegration, SliderDragSetsParam) {
    ComponentRegistry reg;
    make_registry(reg);
    Graph g;
    two_nodes(g, reg);
    auto l = editor_layout::build_layout(g, {});

    EventQueue<std::string> edits;
    SliderDragNode n;
    n.set_context({&g, &edits, nullptr, nullptr});
    const float* tp = nullptr;
    const auto& sl = l.cards[0].sliders[0];  // add0.a
    Eigen::Vector3f right_end = sl.world_pos + Eigen::Vector3f{sl.width * 0.49f, 0, 0};
    aim(n, right_end, 1.f, &tp);
    n.endpoints.trigger.src = tp;
    n(0.0);

    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    std::string json = apply_edit_op(ops[0], g);
    EXPECT_NE(json.find("\"a\":9"), std::string::npos);  // near max (+1000)
}

// dwell_delete → remove_node op → the node disappears.
TEST(EditorIntegration, DwellDeleteRemovesNode) {
    ComponentRegistry reg;
    make_registry(reg);
    Graph g;
    two_nodes(g, reg);
    auto l = editor_layout::build_layout(g, {});

    EventQueue<std::string> edits;
    DwellDeleteNode n;
    n.set_context({&g, &edits, nullptr, nullptr});
    const float* gp = nullptr;
    Eigen::Vector3f body = l.cards[0].position;  // add0 body
    aim(n, body, 1.f, &gp);
    n.endpoints.grip.src = gp;
    n(0.0);
    aim(n, body, 1.f, &gp);
    n.endpoints.grip.src = gp;
    n(1.2);  // dwell > 1 s

    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    std::string json = apply_edit_op(ops[0], g);
    auto g2 = parse_graph(json, reg);
    ASSERT_TRUE(g2);
    EXPECT_EQ(g2->nodes.size(), 1u);
    EXPECT_EQ(g2->nodes[0].id, "mul0");
}

// palette poke → add_node op → a new node of the chosen type appears.
TEST(EditorIntegration, PaletteSpawnsNode) {
    ComponentRegistry reg;
    make_registry(reg);
    Graph g;  // empty
    std::vector<std::string> types = {"add", "mul"};

    EventQueue<std::string> edits;
    PaletteNode n;
    n.set_context({&g, &edits, nullptr, &types});
    const float* tp = nullptr;
    Eigen::Vector3f r1 = PaletteNode::panel_pos();
    float v = (1.f + 0.5f) / float(PaletteNode::kRows + 1);
    r1.y() = PaletteNode::panel_pos().y() + PaletteNode::panel_h() * (0.5f - v);
    aim(n, r1, 0.f, &tp);
    n.endpoints.trigger.src = tp;
    n(0.0);
    aim(n, r1, 1.f, &tp);
    n.endpoints.trigger.src = tp;
    n(0.0);  // poke

    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    std::string json = apply_edit_op(ops[0], g);
    auto g2 = parse_graph(json, reg);
    ASSERT_TRUE(g2);
    ASSERT_EQ(g2->nodes.size(), 1u);
    EXPECT_STREQ(g2->nodes[0].desc->type_name, "add");
}

// A node id with a quote/backslash is escaped at the gesture (json_escape)
// so the op JSON stays structured; apply_edit_op unescapes on its side.
TEST(EditorIntegration, GestureEscapesWeirdIdIntoOp) {
    ComponentRegistry reg;
    make_registry(reg);
    Graph g;
    const EyeballsNodeDescriptor* add = reg.find("add");
    g.nodes.push_back({add, add->create(), "we\"ird\\1"});
    auto l = editor_layout::build_layout(g, {});

    EventQueue<std::string> edits;
    DwellDeleteNode n;
    n.set_context({&g, &edits, nullptr, nullptr});
    const float* gp = nullptr;
    aim(n, l.cards[0].position, 1.f, &gp);
    n.endpoints.grip.src = gp;
    n(0.0);
    aim(n, l.cards[0].position, 1.f, &gp);
    n.endpoints.grip.src = gp;
    n(1.2);  // dwell > 1 s

    auto ops = edits.drain();
    ASSERT_EQ(ops.size(), 1u);
    EXPECT_EQ(ops[0], "{\"op\":\"remove_node\",\"id\":\"we\\\"ird\\\\1\"}");
}

// ── PeerCore-level: the generic v9 context seam + the set_param fast path ──

namespace {
int free_port() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    socklen_t len = sizeof(addr);
    getsockname(s, reinterpret_cast<sockaddr*>(&addr), &len);
    int port = ntohs(addr.sin_port);
    close(s);
    return port;
}

// A context-consuming node with no editor coupling: proves ANY node —
// including one inside a subgraph — receives the host context.
struct CtxProbe {
    static consteval std::string_view name() { return "ctxprobe"; }
    struct endpoints {
        out<float> y;
    } endpoints;
    void operator()(double) {}
    void set_host_context(const char* kind, void* ctx) {
        if (std::string_view{kind} == editor_layout::kEditorContextKind)
            got = static_cast<const editor_layout::GestureContext*>(ctx)->graph;
    }
    const Graph* got = nullptr;
};
}  // namespace

// Fix §1: pump_contexts reaches every descriptor with set_host_context — no
// type dispatch — and subgraphs forward it to their inner nodes.
TEST(EditorIntegration, ContextReachesNodeInsideSubgraph) {
    PeerCore core;
    core.registry.register_builtin(make_descriptor<CtxProbe>());
    PeerCore::Config cfg;
    cfg.http_port = free_port();
    cfg.default_graph_json = R"({"nodes":[
        {"id":"top","type":"ctxprobe"},
        {"id":"sub","graph":{"nodes":[{"id":"inner","type":"ctxprobe"}],"edges":[]}}
    ],"edges":[]})";
    core.init(cfg);
    ASSERT_NE(core.graph(), nullptr);
    core.pump_contexts(1.0f);

    auto* top = static_cast<CtxProbe*>(core.graph()->nodes[0].data);
    EXPECT_EQ(top->got, core.graph());
    auto* sub = static_cast<SubgraphNode*>(core.graph()->nodes[1].data);
    auto* inner = static_cast<CtxProbe*>(sub->inner().nodes[0].data);
    EXPECT_EQ(inner->got, core.graph());  // top-level-only no more
}

// Fix §2: a set_param op takes the in-place queue_param path — the active
// Graph pointer survives (no rebuild/swap) and the param lands.
TEST(EditorIntegration, SetParamOpDoesNotRebuildGraph) {
    PeerCore core;
    core.registry.register_builtin(make_descriptor<AddNode>());
    PeerCore::Config cfg;
    cfg.http_port = free_port();
    cfg.default_graph_json = R"({"nodes":[{"id":"add0","type":"add"}],"edges":[]})";
    core.init(cfg);
    const Graph* before = core.graph();
    ASSERT_NE(before, nullptr);

    // A burst (slider drag) coalesces to the last value.
    core.queue_edit(R"({"op":"set_param","id":"add0","port":"a","value":2.5})");
    core.queue_edit(R"({"op":"set_param","id":"add0","port":"a","value":7.25})");
    core.collect_edits();
    core.begin_frame();

    EXPECT_EQ(core.graph(), before);  // no swap: caches/pointers stay valid
    const auto& n = core.graph()->nodes[0];
    const char* s = n.desc->serialize(n.data);
    std::string j{s};
    n.desc->free_str(s);
    EXPECT_NE(j.find("\"a\":7.25"), std::string::npos);

    // A structural op still rebuilds via apply_edit_op + swap.
    core.queue_edit(R"({"op":"add_node","type":"add"})");
    core.collect_edits();
    core.begin_frame();
    EXPECT_NE(core.graph(), before);
    EXPECT_EQ(core.graph()->nodes.size(), 2u);
}
