// Copyright 2025 Travis West
// Integration tests: real visual node types through ABI v4 and signal_graph.
#include "host_gl_context.hpp"
#include "signal_graph.hpp"
#include "component_registry.hpp"
#include "eyeballs_node_abi.hpp"
#include "eyeballs_node_abi.h"
#include "port_schema_reader.hpp"
#include "water_surface.hpp"
#include "sky_dome.hpp"
#include "lissajous.hpp"
#include "chladni.hpp"
#include "reaction_diffusion.hpp"
#include "aurora.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Core>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>

// ── GL context fixture ────────────────────────────────────────────────────────
// All visual node tests require a current GL context. Create it once for
// the whole suite so we don't pay EGL init overhead per test.

class RealNodesTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        ctx_ = HostGlContext::create();
    }
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
    // slider name is "cell size" (with space) per water_surface.hpp
    d->set_scalar_in(node, "cell size", 3.14);
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

    bool has_render = false, has_elev_out = false;
    for (auto& p : schema.outputs) {
        if (p.name == "render")           has_render   = true;
        if (p.name == "sun_elevation_out") has_elev_out = true;
    }
    EXPECT_TRUE(has_render)    << "port_schema missing 'render' output";
    EXPECT_TRUE(has_elev_out)  << "port_schema missing 'sun_elevation_out' output";
}

TEST_F(RealNodesTest, SkyDomePushOutputs) {
    ASSERT_TRUE(ctx_.has_value());
    auto* d = make_descriptor<SkyDome>();
    ASSERT_NE(d->push_outputs, nullptr);

    void* node = d->create();
    d->process(node, 0.0);

    std::vector<std::string> scalar_ports;
    EyeballsOutputCtx ctx{};
    ctx.store   = &scalar_ports;
    ctx.node_id = "sky";
    ctx.emit_scalar = [](void* store, const char*, const char* port, double) {
        static_cast<std::vector<std::string>*>(store)->push_back(port);
    };
    ctx.emit_vec2 = [](void*, const char*, const char*, float, float) {};
    ctx.emit_vec3 = [](void*, const char*, const char*, float, float, float) {};
    ctx.emit_vec4 = [](void*, const char*, const char*, float, float, float, float) {};
    ctx.emit_mat4 = [](void*, const char*, const char*, const float*) {};
    ctx.emit_quat = [](void*, const char*, const char*, float, float, float, float) {};
    ctx.emit_texture = [](void*, const char*, const char*, unsigned int, int, int,
                          unsigned int, unsigned int) {};
    ctx.emit_audio = [](void*, const char*, const char*, const float*, int, int, int) {};

    d->push_outputs(node, &ctx);

    bool found = false;
    for (auto& p : scalar_ports)
        if (p == "sun_elevation_out") { found = true; break; }
    EXPECT_TRUE(found) << "push_outputs did not emit sun_elevation_out; emitted: "
        << [&]{ std::string r; for (auto& p : scalar_ports) r += p + " "; return r; }();

    d->destroy(node);
}

TEST_F(RealNodesTest, DefaultGraphParsesAndTicks) {
    ASSERT_TRUE(ctx_.has_value());
    static auto* sky_desc   = make_descriptor<SkyDome>();
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

    tick_graph(*g, 0.0);
    EXPECT_EQ(g->draw_calls.size(), 2u);
}

TEST_F(RealNodesTest, EdgePropagationSkyToWater) {
    ASSERT_TRUE(ctx_.has_value());
    static auto* sky_desc   = make_descriptor<SkyDome>();
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
    ASSERT_GT(g->values.count("sky.sun_elevation_out"), 0u)
        << "sky.sun_elevation_out not in value store after tick";
    auto& v = g->values.at("sky.sun_elevation_out");
    EXPECT_TRUE(std::holds_alternative<double>(v))
        << "Expected double in value store for sun_elevation_out";
    // The value should be within the slider range [-1, 1]
    EXPECT_GE(std::get<double>(v), -1.0);
    EXPECT_LE(std::get<double>(v),  1.0);
}

TEST_F(RealNodesTest, AllVisualNodesHaveDrawCalls) {
    ASSERT_TRUE(ctx_.has_value());
    struct NodeDesc { const char* type; const EyeballsNodeDescriptor* desc; };
    static NodeDesc nodes[] = {
        { "sky_dome",           make_descriptor<SkyDome>() },
        { "water_surface",      make_descriptor<WaterSurface>() },
        { "lissajous",          make_descriptor<Lissajous>() },
        { "chladni",            make_descriptor<Chladni>() },
        { "reaction_diffusion", make_descriptor<ReactionDiffusion>() },
        { "aurora",             make_descriptor<Aurora>() },
    };

    for (auto& nd : nodes) {
        ComponentRegistry reg;
        reg.register_builtin(nd.desc);

        std::string json = std::string(R"({"nodes":[{"id":"n","type":")") +
                           nd.type + R"(","params":{}}],"edges":[]})";
        auto g = parse_graph(json, reg);
        ASSERT_NE(g, nullptr) << "parse_graph failed for " << nd.type;

        tick_graph(*g, 0.0);
        EXPECT_EQ(g->draw_calls.size(), 1u) << nd.type << " did not produce a draw call";
    }
}

TEST_F(RealNodesTest, DrawCallNoGlError) {
    ASSERT_TRUE(ctx_.has_value());
    // Clear any existing GL error before the test
    while (glGetError() != GL_NO_ERROR) {}

    static auto* water_desc = make_descriptor<WaterSurface>();
    ComponentRegistry reg;
    reg.register_builtin(water_desc);

    auto g = parse_graph(
        R"({"nodes":[{"id":"w","type":"water_surface","params":{}}],"edges":[]})", reg);
    ASSERT_NE(g, nullptr);

    tick_graph(*g, 0.0);
    ASSERT_EQ(g->draw_calls.size(), 1u);

    g->draw_calls[0](Eigen::Matrix4f::Identity());

    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_NO_ERROR));
}

TEST_F(RealNodesTest, GraphSerializeRoundTrip) {
    ASSERT_TRUE(ctx_.has_value());
    static auto* sky_desc   = make_descriptor<SkyDome>();
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
    EXPECT_EQ(g2->edges[0].to_node,   "water");
}
