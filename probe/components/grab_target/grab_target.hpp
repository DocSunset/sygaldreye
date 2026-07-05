#pragma once
#include <Eigen/Core>

struct GrabTarget {
    Eigen::Vector3f position    = Eigen::Vector3f::Zero();
    float           radius      = 0.05F;
    bool            grabbed     = false;
    int             grabbing_hand = -1;
    Eigen::Vector3f grab_offset = Eigen::Vector3f::Zero();
};
