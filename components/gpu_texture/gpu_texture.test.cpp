// Copyright 2025 Travis West
#include "gpu_texture.hpp"
#include <gtest/gtest.h>

TEST(GpuTexture, DefaultInvalid) {
    GpuTexture t{};
    EXPECT_FALSE(t.valid());
}

TEST(GpuTexture, ValidWhenIdSet) {
    GpuTexture t{};
    t.id = 1;
    EXPECT_TRUE(t.valid());
}

TEST(TextureOutput, NameAndFormat) {
    texture_output<"albedo"> port{};
    EXPECT_EQ(port.name(), "albedo");
    EXPECT_EQ(texture_output<"albedo">::internal_format, 0x881Au);
    EXPECT_FALSE(port.value.valid());
}
