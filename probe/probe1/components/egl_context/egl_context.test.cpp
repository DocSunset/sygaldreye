#include "egl_context.hpp"
#include <GLES3/gl3.h>
#include <gtest/gtest.h>
#include <cstdio>

TEST(egl_context, init_and_gles3_context) {
    EglContext ctx;
    ASSERT_TRUE(ctx.init());
    const char* version  = (const char*)glGetString(GL_VERSION);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* vendor   = (const char*)glGetString(GL_VENDOR);
    ASSERT_NE(version,  nullptr);
    ASSERT_NE(renderer, nullptr);
    ASSERT_NE(vendor,   nullptr);
    printf("GL_VERSION:  %s\n", version);
    printf("GL_RENDERER: %s\n", renderer);
    printf("GL_VENDOR:   %s\n", vendor);
    EXPECT_NE(std::string(version).find("OpenGL ES 3"), std::string::npos);
}
