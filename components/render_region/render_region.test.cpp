// Copyright 2026 Travis West
// Cache behavior of the GL boundary: versioned geometry, content-keyed
// programs, eviction. Needs a headless EGL context; skips if unavailable.
#include "render_region.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <optional>

#include "common_shaders.hpp"
#include "host_gl_context.hpp"
#include "tri_mesh.hpp"

namespace {

std::shared_ptr<TriMeshData> one_triangle() {
    auto d = std::make_shared<TriMeshData>();
    d->vertices = {
        {{0.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 0.f, 0.f, 1.f}},
        {{1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 1.f, 0.f, 1.f}},
        {{0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f, 1.f}},
    };
    d->indices = {0, 1, 2};
    return d;
}

Shader unlit_shader() {
    return std::make_shared<ShaderData>(ShaderData{
        common_shaders::kUnlitVertexColorVert, common_shaders::kUnlitVertexColorFrag});
}

class RenderRegionTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() { ctx_ = HostGlContext::create(); }
    static void TearDownTestSuite() { ctx_.reset(); }
    void SetUp() override {
        if (!ctx_) GTEST_SKIP() << "no headless EGL context";
    }
    // One frame: begin, enqueue, issue per eye (twice, like the Quest shell).
    static void frame(RenderRegion& rr, const Mesh* mesh, const Surface* surf, int eyes = 2) {
        rr.begin_frame();
        if (mesh) rr.enqueue(*mesh, *surf);
        const Eigen::Matrix4f i = Eigen::Matrix4f::Identity();
        for (int e = 0; e < eyes; ++e) rr.issue(i, i, 0.0);
    }
    static std::optional<HostGlContext> ctx_;
};
std::optional<HostGlContext> RenderRegionTest::ctx_;

TEST_F(RenderRegionTest, SameVersionUploadsOnceAcrossEyesAndFrames) {
    auto& rr = RenderRegion::instance();
    auto geo = one_triangle();
    Mesh m;
    m.geometry = geo;
    Surface s;
    s.shader = unlit_shader();

    const auto before = rr.stats().geometry_uploads;
    frame(rr, &m, &s);  // two eyes
    EXPECT_EQ(rr.stats().geometry_uploads, before + 1);
    frame(rr, &m, &s);  // next frame, unchanged version → bind only
    EXPECT_EQ(rr.stats().geometry_uploads, before + 1);
}

TEST_F(RenderRegionTest, TouchForcesReupload) {
    auto& rr = RenderRegion::instance();
    auto geo = one_triangle();
    Mesh m;
    m.geometry = geo;
    Surface s;
    s.shader = unlit_shader();

    frame(rr, &m, &s);
    const auto uploaded = rr.stats().geometry_uploads;
    geo->vertices[0].position.x() = 0.5f;
    geo->touch();
    frame(rr, &m, &s);
    EXPECT_EQ(rr.stats().geometry_uploads, uploaded + 1);
}

TEST_F(RenderRegionTest, IdenticalShaderSourceCompilesOnce) {
    auto& rr = RenderRegion::instance();
    auto geo = one_triangle();
    Mesh m;
    m.geometry = geo;
    // Source unique to this test (other tests may have warmed the cache with
    // the common shader); two DISTINCT ShaderData, identical content.
    const std::string vert =
        std::string("//dedupe\n") + common_shaders::kUnlitVertexColorVert;
    Surface a, b;
    a.shader = std::make_shared<ShaderData>(
        ShaderData{vert, common_shaders::kUnlitVertexColorFrag});
    b.shader = std::make_shared<ShaderData>(
        ShaderData{vert, common_shaders::kUnlitVertexColorFrag});

    const auto before = rr.stats().program_compiles;
    rr.begin_frame();
    rr.enqueue(m, a);
    rr.enqueue(m, b);
    const Eigen::Matrix4f i = Eigen::Matrix4f::Identity();
    rr.issue(i, i, 0.0);
    EXPECT_EQ(rr.stats().program_compiles, before + 1);
}

TEST_F(RenderRegionTest, DeadGeometryIsEvictedPromptly) {
    auto& rr = RenderRegion::instance();
    Surface s;
    s.shader = unlit_shader();
    {
        auto geo = one_triangle();
        Mesh m;
        m.geometry = geo;
        frame(rr, &m, &s);
        EXPECT_GE(rr.cached_geometries(), 1u);
    }  // last owner gone
    frame(rr, nullptr, nullptr);  // next frame's evict sweep sees it expired
    frame(rr, nullptr, nullptr);
    EXPECT_EQ(rr.cached_geometries(), 0u);
}

TEST_F(RenderRegionTest, UnusedProgramsAgeOut) {
    auto& rr = RenderRegion::instance();
    auto geo = one_triangle();
    Mesh m;
    m.geometry = geo;
    Surface s;
    s.shader = unlit_shader();
    frame(rr, &m, &s);
    EXPECT_GE(rr.cached_programs(), 1u);
    for (int f = 0; f < 302; ++f) frame(rr, nullptr, nullptr);
    EXPECT_EQ(rr.cached_programs(), 0u);
}

}  // namespace
