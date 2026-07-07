#pragma once
#include <Eigen/Core>
#include <cstdint>

enum class LightType : uint8_t { Directional };

struct Light {
    LightType       type      = LightType::Directional;
    Eigen::Vector3f direction = Eigen::Vector3f::UnitZ();
    Eigen::Vector3f color     = Eigen::Vector3f::Ones();
    float           intensity = 1.0F;
};
