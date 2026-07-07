#pragma once
#include <Eigen/Core>

struct Material {
    Eigen::Vector3f ambient  = {0.1F, 0.1F, 0.1F};
    Eigen::Vector3f diffuse  = {0.8F, 0.8F, 0.8F};
    Eigen::Vector3f specular = {0.5F, 0.5F, 0.5F};
    float shininess          = 32.0F;
};
