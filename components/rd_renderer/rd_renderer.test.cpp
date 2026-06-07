// Copyright 2025 Travis West
#include "rd_renderer.hpp"
#include "host_gl_context.hpp"
#include <GLES3/gl3.h>
#include <gtest/gtest.h>

TEST(RDRenderer, CreateNoGlError) {
    auto ctx = HostGlContext::create();
    ASSERT_TRUE(ctx.has_value());
    auto r = RDRenderer::create();
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_NO_ERROR));
}

TEST(RDRenderer, DrawWithInvalidTextureIsNoop) {
    auto ctx = HostGlContext::create();
    ASSERT_TRUE(ctx.has_value());
    auto r = RDRenderer::create();
    glGetError(); // clear
    r.draw({});   // texture_.valid() == false — should be no-op
    EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_NO_ERROR));
}
