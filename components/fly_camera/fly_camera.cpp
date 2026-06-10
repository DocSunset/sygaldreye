// Copyright 2026 Travis West
#include "fly_camera.hpp"
#include <cmath>

Eigen::Vector3f FlyCamera::forward() const {
    float cp = std::cos(pitch);
    return {-std::sin(yaw) * cp, std::sin(pitch), -std::cos(yaw) * cp};
}

Eigen::Vector3f FlyCamera::right() const {
    return {std::cos(yaw), 0.f, -std::sin(yaw)};
}

Eigen::Matrix4f FlyCamera::view() const {
    float cy = std::cos(yaw),   sy = std::sin(yaw);
    float cp = std::cos(pitch), sp = std::sin(pitch);
    Eigen::Matrix4f ry = Eigen::Matrix4f::Identity();
    ry(0,0) =  cy; ry(0,2) = -sy;
    ry(2,0) =  sy; ry(2,2) =  cy;
    Eigen::Matrix4f rx = Eigen::Matrix4f::Identity();
    rx(1,1) =  cp; rx(1,2) =  sp;
    rx(2,1) = -sp; rx(2,2) =  cp;
    Eigen::Matrix4f t = Eigen::Matrix4f::Identity();
    t.block<3,1>(0,3) = -pos;
    return rx * ry * t;
}

Eigen::Matrix4f FlyCamera::proj(float aspect) const {
    float near = 0.1f, far = 1000.f;
    float f = 1.f / std::tan(fov / 2.f);
    Eigen::Matrix4f p = Eigen::Matrix4f::Zero();
    p(0,0) = f / aspect;
    p(1,1) = f;
    p(2,2) = (far + near) / (near - far);
    p(2,3) = (2.f * far * near) / (near - far);
    p(3,2) = -1.f;
    return p;
}
