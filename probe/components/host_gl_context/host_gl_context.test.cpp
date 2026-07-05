#include "host_gl_context.hpp"
#include <GLES3/gl3.h>
#include <gtest/gtest.h>

TEST(HostGlContext, CreateSucceeds) {
    auto ctx = HostGlContext::create();
    ASSERT_TRUE(ctx.has_value());
}

TEST(HostGlContext, GlVersionNonNull) {
    auto ctx = HostGlContext::create();
    ASSERT_TRUE(ctx.has_value());
    const auto* version = glGetString(GL_VERSION);
    ASSERT_NE(version, nullptr);
}
