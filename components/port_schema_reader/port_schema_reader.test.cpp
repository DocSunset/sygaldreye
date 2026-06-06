// Copyright 2025 Travis West
#include "port_schema_reader.hpp"
#include <gtest/gtest.h>

TEST(PortSchemaReader, NullReturnsEmpty) {
    auto s = parse_port_schema(nullptr);
    EXPECT_TRUE(s.inputs.empty());
    EXPECT_TRUE(s.outputs.empty());
}

TEST(PortSchemaReader, EmptyArrays) {
    auto s = parse_port_schema("{\"inputs\":[],\"outputs\":[]}");
    EXPECT_TRUE(s.inputs.empty());
    EXPECT_TRUE(s.outputs.empty());
}

TEST(PortSchemaReader, OneInput) {
    auto s = parse_port_schema(
        "{\"inputs\":[{\"name\":\"wave_height\",\"kind\":\"scalar\"}],\"outputs\":[]}");
    ASSERT_EQ(s.inputs.size(), 1u);
    EXPECT_EQ(s.inputs[0].name, "wave_height");
    EXPECT_EQ(s.inputs[0].kind, "scalar");
    EXPECT_TRUE(s.inputs[0].is_wirable());
    EXPECT_FALSE(s.inputs[0].is_drawable());
    EXPECT_TRUE(s.outputs.empty());
}

TEST(PortSchemaReader, MixedPorts) {
    auto s = parse_port_schema(
        "{\"inputs\":[{\"name\":\"sun_dir\",\"kind\":\"vec3\"}],"
        "\"outputs\":[{\"name\":\"render\",\"kind\":\"draw_call\"},"
        "{\"name\":\"elev_out\",\"kind\":\"scalar\"}]}");
    ASSERT_EQ(s.inputs.size(), 1u);
    ASSERT_EQ(s.outputs.size(), 2u);
    EXPECT_EQ(s.outputs[0].kind, "draw_call");
    EXPECT_TRUE(s.outputs[0].is_drawable());
    EXPECT_FALSE(s.outputs[0].is_wirable());
    EXPECT_EQ(s.outputs[1].name, "elev_out");
}
