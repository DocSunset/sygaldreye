// Copyright 2026 Travis West
#include "poke_stick.hpp"

#include <gtest/gtest.h>

// tip_pos math + emitted payloads. No GL — the node emits a declarative
// Mesh+Surface, so the box is inspectable on the host.

TEST(PokeStick, TipIsLengthDownControllerNegativeZ) {
    PokeStickNode s;
    Eigen::Vector3f pos{0.f, 1.f, -0.5f};
    Eigen::Quaternionf rot = Eigen::Quaternionf::Identity();
    s.endpoints.pos.src = &pos;
    s.endpoints.rot.src = &rot;
    s.endpoints.length.fallback = 0.2f;
    s(0.0);
    Eigen::Vector3f expect = pos + Eigen::Vector3f{0.f, 0.f, -0.2f};
    EXPECT_TRUE(s.endpoints.tip_pos.value.isApprox(expect, 1e-5f));
}

TEST(PokeStick, EmitsBoxMeshAndSurface) {
    PokeStickNode s;
    s(0.0);
    auto& m = s.endpoints.mesh.value;
    ASSERT_TRUE(m.geometry);
    EXPECT_EQ(m.geometry->vertices.size(), 24u);
    EXPECT_EQ(m.geometry->indices.size(), 36u);
    EXPECT_TRUE(s.endpoints.surface.value.shader);
}

TEST(PokeStick, ActiveChangesColorNotGeometry) {
    PokeStickNode s;
    s(0.0);
    Eigen::Vector4f idle = std::get<Eigen::Vector4f>(s.endpoints.surface.value.uniforms[0].value);
    s.endpoints.active.fallback = 1.f;
    s(0.0);
    Eigen::Vector4f hot = std::get<Eigen::Vector4f>(s.endpoints.surface.value.uniforms[0].value);
    EXPECT_FALSE(idle.isApprox(hot));
}
