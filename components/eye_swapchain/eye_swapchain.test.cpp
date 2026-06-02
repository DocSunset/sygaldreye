#include "eye_swapchain.hpp"
#include <GLES3/gl3.h>
#include <gtest/gtest.h>

TEST(eye_swapchain, ChooseFormatSrgbPresent) {
    std::vector<int64_t> fmts = {GL_RGBA8, GL_SRGB8_ALPHA8};
    EXPECT_EQ(choose_swapchain_format(fmts), GL_SRGB8_ALPHA8);
}
TEST(eye_swapchain, ChooseFormatRgba8Fallback) {
    std::vector<int64_t> fmts = {GL_RGBA8, 0x1234};
    EXPECT_EQ(choose_swapchain_format(fmts), GL_RGBA8);
}
TEST(eye_swapchain, ChooseFormatFirstFallback) {
    std::vector<int64_t> fmts = {0x1234, 0x5678};
    EXPECT_EQ(choose_swapchain_format(fmts), 0x1234);
}
