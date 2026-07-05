// Copyright 2025 Travis West
#include "gl_geometry.hpp"
#include "egl_context.hpp"
#include <GLES3/gl3.h>
#include <gtest/gtest.h>

namespace {
// triangle: 3 vertices, 6 floats each (xyz + rgb), 2 attribs
constexpr float kVerts[] = {
    0.f, 0.f, 0.f,  1.f, 0.f, 0.f,
    1.f, 0.f, 0.f,  0.f, 1.f, 0.f,
    0.f, 1.f, 0.f,  0.f, 0.f, 1.f,
};
constexpr uint32_t kIndices[] = { 0, 1, 2 };
const std::vector<AttribDesc> kLayout = {
    {0, 3, 6 * sizeof(float), 0},
    {1, 3, 6 * sizeof(float), 3 * sizeof(float)},
};
} // namespace

class GlGeometryTest : public ::testing::Test {
protected:
    EglContext ctx_;
    void SetUp() override { ASSERT_TRUE(ctx_.init()); }
};

TEST_F(GlGeometryTest, CreateNoGlError) {
    glGetError();
    auto g = GlGeometry::create(kVerts, kIndices, kLayout);
    EXPECT_EQ(glGetError(), GLenum(GL_NO_ERROR));
}

TEST_F(GlGeometryTest, UpdateVertsNoGlError) {
    auto g = GlGeometry::create(kVerts, {}, kLayout, GL_DYNAMIC_DRAW);
    glGetError();
    g.update_verts(kVerts);
    EXPECT_EQ(glGetError(), GLenum(GL_NO_ERROR));
}

TEST_F(GlGeometryTest, DrawArraysNoGlError) {
    auto g = GlGeometry::create(kVerts, {}, kLayout, GL_DYNAMIC_DRAW);
    glGetError();
    g.draw_arrays(GL_TRIANGLES, 3);
    EXPECT_EQ(glGetError(), GLenum(GL_NO_ERROR));
}

TEST_F(GlGeometryTest, DrawElementsNoGlError) {
    auto g = GlGeometry::create(kVerts, kIndices, kLayout);
    glGetError();
    g.draw_elements(GL_TRIANGLES, 3);
    EXPECT_EQ(glGetError(), GLenum(GL_NO_ERROR));
}

TEST_F(GlGeometryTest, MoveConstruct) {
    auto a = GlGeometry::create(kVerts, kIndices, kLayout);
    auto b = std::move(a);
    glGetError();
    b.draw_elements(GL_TRIANGLES, 3);
    EXPECT_EQ(glGetError(), GLenum(GL_NO_ERROR));
}
