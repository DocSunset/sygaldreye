// Copyright 2026 Travis West
#pragma once
#include <Eigen/Core>

// First-person flying camera. Pure math; input application lives elsewhere.
struct FlyCamera {
    Eigen::Vector3f pos{0.f, 2.f, 8.f};
    float yaw   = 0.f;  // radians around +Y; 0 looks down -Z
    float pitch = 0.f;  // radians; positive looks up
    float fov   = 0.9f; // vertical, radians

    Eigen::Matrix4f view() const;
    Eigen::Matrix4f proj(float aspect) const;
    Eigen::Vector3f forward() const;
    Eigen::Vector3f right() const;
};
