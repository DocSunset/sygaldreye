#define STB_IMAGE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "stb_image.h"
#pragma GCC diagnostic pop
#include "host_gl_context.hpp"
#include "scene_snapshot.hpp"
#include <GLES3/gl3.h>
#include <gtest/gtest.h>
#include <cstdio>
#include <filesystem>

TEST(SceneSnapshot, SolidColorMatchesClear) {
    auto ctx = HostGlContext::create();
    ASSERT_TRUE(ctx.has_value());

    const char* path = "/tmp/scene_snapshot_test.png";
    SnapshotParams params;
    params.width  = 64;
    params.height = 64;
    params.projection = Eigen::Matrix4f::Identity();
    params.view       = Eigen::Matrix4f::Identity();

    bool ok = write_snapshot(params, [](Eigen::Matrix4f const&, Eigen::Matrix4f const&) {
        glClearColor(0.25f, 0.50f, 0.75f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }, path);
    ASSERT_TRUE(ok);

    int w = 0, h = 0, ch = 0;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
    ASSERT_NE(data, nullptr);
    ASSERT_EQ(w, 64);
    ASSERT_EQ(h, 64);

    for (int i = 0; i < w * h; ++i) {
        EXPECT_NEAR(data[i * 4 + 0],  64, 2);   // 0.25 * 255 ≈ 64
        EXPECT_NEAR(data[i * 4 + 1], 128, 2);   // 0.50 * 255 ≈ 128
        EXPECT_NEAR(data[i * 4 + 2], 191, 2);   // 0.75 * 255 ≈ 191
        EXPECT_EQ  (data[i * 4 + 3], 255);
    }

    stbi_image_free(data);
    std::filesystem::remove(path);
}
