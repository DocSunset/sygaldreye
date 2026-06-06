#pragma once
#include <Eigen/Core>
#include <optional>

struct RgbaShader;

struct RayHit {
    float t;
    Eigen::Vector2f uv;  // [0,1]^2 local panel coords
};

struct VrPanel {
    Eigen::Vector3f position = {0, 0, -1};
    Eigen::Vector3f normal   = {0, 0,  1};
    float width  = 0.4f;
    float height = 0.3f;
    Eigen::Vector4f color = {0.1f, 0.1f, 0.15f, 0.85f};

    // Returns hit if ray from origin in direction dir intersects this panel.
    std::optional<RayHit> intersect(const Eigen::Vector3f& origin,
                                    const Eigen::Vector3f& dir) const;

    // Draw the panel quad. Requires GL context current.
    void draw(const Eigen::Matrix4f& vp, RgbaShader& shader) const;
};
