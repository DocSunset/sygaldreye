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
    EXPECT_TRUE(s.outputs.empty());
}

TEST(PortSchemaReader, CellRankFromKind) {
    auto s = parse_port_schema(
        "{\"inputs\":["
        "{\"name\":\"s\",\"kind\":\"scalar\"},"
        "{\"name\":\"v3\",\"kind\":\"vec3\"},"
        "{\"name\":\"q\",\"kind\":\"quat\"},"
        "{\"name\":\"m\",\"kind\":\"mat4\"},"
        "{\"name\":\"a\",\"kind\":\"audio\"}],\"outputs\":[]}");
    ASSERT_EQ(s.inputs.size(), 5u);
    EXPECT_EQ(s.inputs[0].cell_rank(), 1);
    EXPECT_EQ(s.inputs[1].cell_rank(), 3);
    EXPECT_EQ(s.inputs[2].cell_rank(), 4);
    EXPECT_EQ(s.inputs[3].cell_rank(), 16);
    EXPECT_EQ(s.inputs[4].cell_rank(), 0);  // audio is whole-by-kind
    EXPECT_FALSE(s.inputs[0].whole);        // a scalar lifts
    EXPECT_TRUE(s.inputs[4].whole);         // audio never lifts
}

TEST(PortSchemaReader, WholeOptOutOnACellKind) {
    // A vec3 port can opt out of lifting via the schema "whole":1 marker.
    auto s = parse_port_schema(
        "{\"inputs\":[{\"name\":\"flock\",\"kind\":\"vec3\",\"whole\":1}],\"outputs\":[]}");
    ASSERT_EQ(s.inputs.size(), 1u);
    EXPECT_EQ(s.inputs[0].cell_rank(), 3);  // rank still derived from kind
    EXPECT_TRUE(s.inputs[0].whole);         // but consumes the array whole
}

TEST(PortSchemaReader, MixedPorts) {
    auto s = parse_port_schema(
        "{\"inputs\":[{\"name\":\"sun_dir\",\"kind\":\"vec3\"}],"
        "\"outputs\":[{\"name\":\"mesh\",\"kind\":\"mesh\"},"
        "{\"name\":\"elev_out\",\"kind\":\"scalar\"}]}");
    ASSERT_EQ(s.inputs.size(), 1u);
    ASSERT_EQ(s.outputs.size(), 2u);
    EXPECT_EQ(s.outputs[0].kind, "mesh");
    EXPECT_TRUE(s.outputs[0].is_wirable());  // every port wires now
    EXPECT_EQ(s.outputs[1].name, "elev_out");
}
