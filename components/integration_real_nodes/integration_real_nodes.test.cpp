// Copyright 2025 Travis West
// Integration tests: real visual node types through ABI v4 and signal_graph.
#include <GLES3/gl3.h>
#include <gtest/gtest.h>

#include <Eigen/Core>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "chladni.hpp"
#include "component_registry.hpp"
#include "eyeballs_node_abi.h"
#include "eyeballs_node_abi.hpp"
#include "host_gl_context.hpp"
#include "lissajous.hpp"
#include "mesh_nodes.hpp"
#include "port_schema_reader.hpp"
#include "reaction_diffusion.hpp"
#include "render_payloads.hpp"
#include "signal_graph.hpp"
#include "signal_graph_plan.hpp"
#include "sky_dome.hpp"
#include "subgraph_node.hpp"
#include "tree_generator_node.hpp"
#include "water_surface.hpp"

// ── GL context fixture ────────────────────────────────────────────────────────
// All visual node tests require a current GL context. Create it once for
// the whole suite so we don't pay EGL init overhead per test.

class RealNodesTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { ctx_ = HostGlContext::create(); }
    static std::optional<HostGlContext> ctx_;
};
std::optional<HostGlContext> RealNodesTest::ctx_;

// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RealNodesTest, GlContextRequired) {
    ASSERT_TRUE(ctx_.has_value()) << "Mesa GL context unavailable — cannot run visual node tests";
}

TEST_F(RealNodesTest, WaterSurfaceDescriptorLifecycle) {
    ASSERT_TRUE(ctx_.has_value());
    auto* d = make_descriptor<WaterSurface>();
    ASSERT_NE(d, nullptr);
    void* node = d->create();
    ASSERT_NE(node, nullptr);
    d->process(node, 0.0);
    const char* json = d->serialize(node);
    ASSERT_NE(json, nullptr);
    // PFR serializes using the C++ field name ("cell_size"), not the slider display name
    EXPECT_NE(std::string(json).find("cell_size"), std::string::npos);
    d->free_str(json);
    d->destroy(node);
}

TEST_F(RealNodesTest, WaterSurfaceSetScalarIn) {
    ASSERT_TRUE(ctx_.has_value());
    auto* d = make_descriptor<WaterSurface>();
    ASSERT_NE(d->set_scalar_in, nullptr);
    void* node = d->create();
    d->set_scalar_in(node, "cell_size", 3.14);
    const char* json = d->serialize(node);
    ASSERT_NE(json, nullptr);
    std::string s(json);
    EXPECT_NE(s.find("3.14"), std::string::npos)
        << "set_scalar_in did not update cell_size; json=" << s;
    d->free_str(json);
    d->destroy(node);
}

TEST_F(RealNodesTest, SkyDomePortSchema) {
    ASSERT_TRUE(ctx_.has_value());
    auto* d = make_descriptor<SkyDome>();
    ASSERT_NE(d->port_schema, nullptr);

    auto schema = parse_port_schema(d->port_schema);

    // Migrated to render-as-nodes: emits surface + mesh, not a draw_call.
    bool has_surface = false, has_mesh = false, has_elev_out = false;
    for (auto& p : schema.outputs) {
        if (p.name == "surface") has_surface = true;
        if (p.name == "mesh") has_mesh = true;
        if (p.name == "sun_elevation_out") has_elev_out = true;
    }
    EXPECT_TRUE(has_surface) << "port_schema missing 'surface' output";
    EXPECT_TRUE(has_mesh) << "port_schema missing 'mesh' output";
    EXPECT_TRUE(has_elev_out) << "port_schema missing 'sun_elevation_out' output";
}

TEST_F(RealNodesTest, SkyDomePushOutputs) {
    ASSERT_TRUE(ctx_.has_value());
    auto* d = make_descriptor<SkyDome>();
    ASSERT_NE(d->push_outputs, nullptr);

    void* node = d->create();
    d->process(node, 0.0);

    std::vector<std::string> scalar_ports;
    EyeballsOutputCtx ctx{};
    ctx.store = &scalar_ports;
    ctx.node_id = "sky";
    ctx.emit_scalar = [](void* store, const char*, const char* port, double) {
        static_cast<std::vector<std::string>*>(store)->push_back(port);
    };
    ctx.emit_vec2 = [](void*, const char*, const char*, float, float) {};
    ctx.emit_vec3 = [](void*, const char*, const char*, float, float, float) {};
    ctx.emit_vec4 = [](void*, const char*, const char*, float, float, float, float) {};
    ctx.emit_mat4 = [](void*, const char*, const char*, const float*) {};
    ctx.emit_quat = [](void*, const char*, const char*, float, float, float, float) {};
    ctx.emit_texture =
        [](void*, const char*, const char*, unsigned int, int, int, unsigned int, unsigned int) {};
    ctx.emit_audio = [](void*, const char*, const char*, const float*, int, int, int) {};

    d->push_outputs(node, &ctx);

    bool found = false;
    for (auto& p : scalar_ports)
        if (p == "sun_elevation_out") {
            found = true;
            break;
        }
    EXPECT_TRUE(found) << "push_outputs did not emit sun_elevation_out; emitted: " << [&] {
        std::string r;
        for (auto& p : scalar_ports) r += p + " ";
        return r;
    }();

    d->destroy(node);
}

TEST_F(RealNodesTest, DefaultGraphParsesAndTicks) {
    ASSERT_TRUE(ctx_.has_value());
    static auto* sky_desc = make_descriptor<SkyDome>();
    static auto* water_desc = make_descriptor<WaterSurface>();

    ComponentRegistry reg;
    reg.register_builtin(sky_desc);
    reg.register_builtin(water_desc);

    const char* json = R"({
        "nodes":[
            {"id":"sky",   "type":"sky_dome",     "params":{}},
            {"id":"water", "type":"water_surface", "params":{}}
        ],
        "edges":[]
    })";

    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);
    EXPECT_EQ(g->nodes.size(), 2u);

    tick_graph(*g, 0.0);  // ticks without crashing
    // Both sky and water emit surface/mesh now (no DrawFn — the type is gone).
}

TEST_F(RealNodesTest, EdgePropagationSkyToWater) {
    ASSERT_TRUE(ctx_.has_value());
    static auto* sky_desc = make_descriptor<SkyDome>();
    static auto* water_desc = make_descriptor<WaterSurface>();

    ComponentRegistry reg;
    reg.register_builtin(sky_desc);
    reg.register_builtin(water_desc);

    const char* json = R"({
        "nodes":[
            {"id":"sky",   "type":"sky_dome",     "params":{"sun elevation":0.7}},
            {"id":"water", "type":"water_surface", "params":{}}
        ],
        "edges":[
            {"from":"sky.sun_elevation_out","to":"water.sun elevation"}
        ]
    })";

    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);
    ASSERT_EQ(g->edges.size(), 1u);

    tick_graph(*g, 0.0);

    // The value store should contain a scalar for the sky's output
    auto vals = snapshot_values(*g);
    ASSERT_GT(vals.count("sky.sun_elevation_out"), 0u)
        << "sky.sun_elevation_out not in value store after tick";
    auto& v = vals.at("sky.sun_elevation_out");
    EXPECT_TRUE(std::holds_alternative<double>(v))
        << "Expected double in value store for sun_elevation_out";
    // The value should be within the slider range [-1, 1]
    EXPECT_GE(std::get<double>(v), -1.0);
    EXPECT_LE(std::get<double>(v), 1.0);
}

TEST_F(RealNodesTest, AllVisualNodesEmitDrawables) {
    ASSERT_TRUE(ctx_.has_value());
    struct NodeDesc {
        const char* type;
        const EyeballsNodeDescriptor* desc;
    };
    // Migrated visual nodes: render-as-nodes contract — each emits a surface
    // + mesh the draw node consumes, and produces NO DrawFn closures.
    static NodeDesc nodes[] = {
        {"sky_dome", make_descriptor<SkyDome>()},
        {"water_surface", make_descriptor<WaterSurface>()},
        {"lissajous", make_descriptor<Lissajous>()},
        {"chladni", make_descriptor<Chladni>()},
        {"reaction_diffusion", make_descriptor<ReactionDiffusion>()},
    };

    for (auto& nd : nodes) {
        ComponentRegistry reg;
        reg.register_builtin(nd.desc);

        std::string json = std::string(R"({"nodes":[{"id":"n","type":")") + nd.type +
                           R"(","params":{}}],"edges":[]})";
        auto g = parse_graph(json, reg);
        ASSERT_NE(g, nullptr) << "parse_graph failed for " << nd.type;

        tick_graph(*g, 0.0);
        auto& n = g->nodes[0];
        ASSERT_NE(n.desc->output_ptr, nullptr);
        EXPECT_NE(n.desc->output_ptr(n.data, "surface"), nullptr) << nd.type << " missing surface";
        EXPECT_NE(n.desc->output_ptr(n.data, "mesh"), nullptr) << nd.type << " missing mesh";
    }
}

TEST_F(RealNodesTest, WaterEmitsDrawable) {
    ASSERT_TRUE(ctx_.has_value());
    static auto* water_desc = make_descriptor<WaterSurface>();
    ComponentRegistry reg;
    reg.register_builtin(water_desc);

    auto g =
        parse_graph(R"({"nodes":[{"id":"w","type":"water_surface","params":{}}],"edges":[]})", reg);
    ASSERT_NE(g, nullptr);

    tick_graph(*g, 0.0);

    // water emits surface + mesh outputs the draw node would consume.
    auto& n = g->nodes[0];
    ASSERT_NE(n.desc->output_ptr, nullptr);
    EXPECT_NE(n.desc->output_ptr(n.data, "surface"), nullptr);
    EXPECT_NE(n.desc->output_ptr(n.data, "mesh"), nullptr);
}

// Forest route 1 (conformability.md): N seeds → LIFTED tree_generator → N
// DISTINCT meshes. The executor lifts the generator over the seed array; the
// outputs gather into a MeshArray. No node hand-rolls the replication.
TEST_F(RealNodesTest, ForestRoute1LiftsTreeGeneratorToNDistinctMeshes) {
    ASSERT_TRUE(ctx_.has_value());
    ComponentRegistry reg;
    reg.register_builtin(make_descriptor<SeedsNode>());
    reg.register_builtin(make_descriptor<TreeGeneratorNode>());

    const char* json = R"({
        "nodes":[
            {"id":"seeds","type":"seeds","params":{"count":5,"base":1,"step":1}},
            {"id":"tree","type":"tree_generator","params":{"spread":10}}],
        "edges":[{"from":"seeds.seeds","to":"tree.seed"}]
    })";
    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr) << "forest graph failed to parse";

    tick_graph(*g, 0.0);

    // One LiftGroup: the tree lifted over the 5-seed array → 5 instances.
    g->plan = build_plan(*g);  // inspect the resolved plan
    ASSERT_EQ(g->plan->lift_groups.size(), 1u);
    auto& lg = g->plan->lift_groups.front();
    EXPECT_EQ(lg.out_kind, "mesh");
    EXPECT_EQ(lg.key_port, "seed");

    tick_graph(*g, 0.0);  // tick under the rebuilt plan to gather
    ASSERT_EQ(lg.mesh_gather.size(), 5u) << "expected 5 trees, one per seed";

    // Distinct meshes: different seeds → different geometry (vertex counts or
    // positions differ). At minimum the shared_ptrs are distinct instances.
    std::set<const void*> ptrs;
    for (auto& m : lg.mesh_gather) {
        ASSERT_NE(m, nullptr);
        ASSERT_FALSE(m->vertices.empty());
        ptrs.insert(m.get());
    }
    EXPECT_EQ(ptrs.size(), 5u) << "trees are not distinct mesh instances";
    // Seed 1 vs seed 5 must differ in geometry (distinct trees, not clones):
    // compare the first vertex position (the seed-scattered offset differs).
    const auto& a = lg.mesh_gather.front()->vertices.front().position;
    const auto& b = lg.mesh_gather.back()->vertices.front().position;
    EXPECT_FALSE(a.isApprox(b)) << "distinct seeds produced identical geometry";
}

TEST_F(RealNodesTest, GraphSerializeRoundTrip) {
    ASSERT_TRUE(ctx_.has_value());
    static auto* sky_desc = make_descriptor<SkyDome>();
    static auto* water_desc = make_descriptor<WaterSurface>();

    ComponentRegistry reg;
    reg.register_builtin(sky_desc);
    reg.register_builtin(water_desc);

    const char* json = R"({
        "nodes":[
            {"id":"sky",   "type":"sky_dome",     "params":{}},
            {"id":"water", "type":"water_surface", "params":{}}
        ],
        "edges":[
            {"from":"sky.sun_elevation_out","to":"water.sun elevation"}
        ]
    })";

    auto g = parse_graph(json, reg);
    ASSERT_NE(g, nullptr);

    std::string serialized = serialize_graph(*g);

    auto g2 = parse_graph(serialized, reg);
    ASSERT_NE(g2, nullptr) << "re-parse of serialized graph failed; json=" << serialized;
    EXPECT_EQ(g2->nodes.size(), 2u);
    EXPECT_EQ(g2->edges.size(), 1u);
    EXPECT_EQ(g2->edges[0].from_node, "sky");
    EXPECT_EQ(g2->edges[0].from_port, "sun_elevation_out");
    EXPECT_EQ(g2->edges[0].to_node, "water");
}
