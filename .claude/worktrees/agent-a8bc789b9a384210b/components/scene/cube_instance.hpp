#pragma once
#include <Eigen/Core>
#include "material.hpp"

struct CubeInstance {
    Eigen::Matrix4f model    = Eigen::Matrix4f::Identity();
    Material        material = {};
};
