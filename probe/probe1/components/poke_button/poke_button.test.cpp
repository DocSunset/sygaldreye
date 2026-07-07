// Copyright 2026 Travis West
#include "poke_button.hpp"

#include <gtest/gtest.h>

// Interaction + emitted payloads. No GL: the node hands render_region a
// declarative Mesh+Surface, so geometry is inspectable on the host.

struct ButtonRig {
    PokeButtonNode b;
    Eigen::Vector3f tip{9.f, 9.f, 9.f};
    float press = 0.f;
    ButtonRig() {
        b.endpoints.x.fallback = 0.f;
        b.endpoints.y.fallback = 1.2f;
        b.endpoints.z.fallback = -0.5f;
        b.endpoints.size.fallback = 0.1f;
        b.endpoints.tip.src = &tip;
        b.endpoints.press.src = &press;
    }
};

TEST(PokeButton, HoverWhenTipInside) {
    ButtonRig r;
    r.b(0.0);
    EXPECT_FLOAT_EQ(r.b.endpoints.hover.value, 0.f);
    r.tip = {0.f, 1.2f, -0.5f};
    r.b(0.0);
    EXPECT_FLOAT_EQ(r.b.endpoints.hover.value, 1.f);
}

TEST(PokeButton, FiresOnPressRiseInside) {
    ButtonRig r;
    r.tip = {0.f, 1.2f, -0.5f};
    r.b(0.0);
    EXPECT_FALSE(r.b.endpoints.pressed.triggered);
    r.press = 1.f;
    r.b(0.0);
    EXPECT_TRUE(r.b.endpoints.pressed.triggered);  // rising edge
    r.b(0.0);
    EXPECT_FALSE(r.b.endpoints.pressed.triggered);  // held, no re-fire
}

TEST(PokeButton, NoFireOutsideEvenWhenPressed) {
    ButtonRig r;
    r.press = 1.f;
    r.b(0.0);
    EXPECT_FALSE(r.b.endpoints.pressed.triggered);
}

TEST(PokeButton, EmitsBoxMeshAndSurface) {
    ButtonRig r;
    r.b(0.0);
    auto& m = r.b.endpoints.mesh.value;
    ASSERT_TRUE(m.geometry);
    EXPECT_EQ(m.geometry->vertices.size(), 24u);  // 6 faces * 4
    EXPECT_EQ(m.geometry->indices.size(), 36u);
    EXPECT_TRUE(r.b.endpoints.surface.value.shader);
    EXPECT_EQ(r.b.endpoints.surface.value.uniforms.size(), 1u);  // uColor
}
