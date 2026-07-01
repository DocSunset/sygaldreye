#include "cylinder_mesh.hpp"

#include <gtest/gtest.h>

TEST(CylinderMesh, GeometryHasSideAndCaps) {
    MeshPtr m = make_cylinder(12);
    ASSERT_TRUE(m);
    // side: 2*12; caps: 2*(1+12)
    EXPECT_EQ(m->vertices.size(), 2u * 12u + 2u * (1u + 12u));
    // side: 12*6; caps: 2*12*3
    EXPECT_EQ(m->indices.size(), 12u * 6u + 2u * 12u * 3u);
}

TEST(CylinderMesh, SideNormalsAreRadial) {
    MeshPtr m = make_cylinder(8);
    // First side vertex at theta=0 sits at (1,0,0.5); normal must be (1,0,0).
    EXPECT_TRUE(m->vertices[0].normal.isApprox(Eigen::Vector3f(1.f, 0.f, 0.f), 1e-5f));
}

TEST(CylinderMesh, TransformMapsUnitOntoSegment) {
    Eigen::Vector3f a{0.f, 0.f, 0.f}, b{0.f, 0.f, 2.f};
    Eigen::Matrix4f xf = cylinder_transform(a, b, 0.5f);
    // Unit +Z end (0,0,0.5) maps to b; -Z end maps to a; length=2.
    Eigen::Vector4f top = xf * Eigen::Vector4f(0.f, 0.f, 0.5f, 1.f);
    Eigen::Vector4f bot = xf * Eigen::Vector4f(0.f, 0.f, -0.5f, 1.f);
    EXPECT_TRUE(top.head<3>().isApprox(b, 1e-5f));
    EXPECT_TRUE(bot.head<3>().isApprox(a, 1e-5f));
}
