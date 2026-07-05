// Copyright 2026 Travis West
#include "port_types.hpp"

#include <gtest/gtest.h>

using namespace port_types;

TEST(PortTypes, EqualKindsAreTrueEdges) {
    EXPECT_TRUE(connection_legal("scalar", "scalar"));
    EXPECT_EQ(boundary_mapping("scalar", "scalar"), "");
    EXPECT_EQ(boundary_mapping("mesh", "mesh"), "");
}

TEST(PortTypes, MismatchedKindsAreIllegal) {
    EXPECT_FALSE(connection_legal("mesh", "audio"));
    EXPECT_EQ(boundary_mapping("mesh", "audio"), std::nullopt);
    EXPECT_FALSE(connection_legal("scalar", "vec3"));
}

TEST(PortTypes, WildcardsAlwaysConnect) {
    EXPECT_TRUE(connection_legal("unknown", "scalar"));  // subgraph outlets
    EXPECT_TRUE(connection_legal("vec3", "any"));        // throughpoints
    EXPECT_EQ(boundary_mapping("any", "audio"), "");
}

TEST(PortTypes, RateBoundariesNameTheirMapping) {
    EXPECT_EQ(rate_of("audio"), Rate::Block);
    EXPECT_EQ(rate_of("scalar"), Rate::Frame);
    // same-kind connections never cross rates in v1; the latch/snapshot
    // rows activate when per-port rates land (audio-region work)
}
