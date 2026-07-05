#pragma once
#include <Eigen/Core>

Eigen::Vector3f tethered_point(Eigen::Vector3f anchor,
                                Eigen::Vector3f desired,
                                float min_dist,
                                float max_dist);
