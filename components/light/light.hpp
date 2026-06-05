#pragma once
#include <Eigen/Core>

enum class LightType { Directional };

struct Light {
    LightType       type      = LightType::Directional;
    Eigen::Vector3f direction = Eigen::Vector3f::UnitZ();
    Eigen::Vector3f color     = Eigen::Vector3f::Ones();
    float           intensity = 1.0F;
};
