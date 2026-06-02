#include "renderer.hpp"
#include <GLES3/gl3.h>
#include <gtest/gtest.h>
#include <cstdio>

TEST(renderer, ChooseFormatSrgbPresent) {
    std::vector<int64_t> fmts = {GL_RGBA8, GL_SRGB8_ALPHA8};
    EXPECT_EQ(choose_swapchain_format(fmts), GL_SRGB8_ALPHA8);
}

TEST(renderer, ChooseFormatRgba8Fallback) {
    std::vector<int64_t> fmts = {GL_RGBA8, 0x1234};
    EXPECT_EQ(choose_swapchain_format(fmts), GL_RGBA8);
}

TEST(renderer, ChooseFormatFirstFallback) {
    std::vector<int64_t> fmts = {0x1234, 0x5678};
    EXPECT_EQ(choose_swapchain_format(fmts), 0x1234);
}

TEST(renderer, egl_init_and_gles3_context) {
    Renderer r;
    ASSERT_TRUE(r.init());
    const char* version  = (const char*)glGetString(GL_VERSION);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* vendor   = (const char*)glGetString(GL_VENDOR);
    ASSERT_NE(version,  nullptr);
    ASSERT_NE(renderer, nullptr);
    ASSERT_NE(vendor,   nullptr);
    printf("GL_VERSION:  %s\n", version);
    printf("GL_RENDERER: %s\n", renderer);
    printf("GL_VENDOR:   %s\n", vendor);
    // Must be ES 3.x
    EXPECT_NE(std::string(version).find("OpenGL ES 3"), std::string::npos);
}
