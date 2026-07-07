#pragma once
#include <Eigen/Core>
#include <string>

struct Label {
    std::string     text;
    Eigen::Matrix4f transform = Eigen::Matrix4f::Identity();
};
