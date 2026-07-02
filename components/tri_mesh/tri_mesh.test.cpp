#include "tri_mesh.hpp"

#include <gtest/gtest.h>

TEST(TriMeshData, FreshAllocationsGetUniqueVersions) {
    TriMeshData a, b;
    EXPECT_NE(a.version, 0u);
    EXPECT_NE(a.version, b.version);
}

TEST(TriMeshData, TouchRestampsMonotonically) {
    TriMeshData a;
    const uint64_t v0 = a.version;
    a.touch();
    EXPECT_GT(a.version, v0);
    a.touch();
    EXPECT_GT(a.version, v0 + 1);
}

TEST(TriMeshData, VersionSurvivesCopyButNotReconstruction) {
    TriMeshData a;
    TriMeshData copy = a;  // copies stamp: same content, same identity
    EXPECT_EQ(copy.version, a.version);
    TriMeshData fresh;  // new object, new stamp
    EXPECT_NE(fresh.version, a.version);
}
