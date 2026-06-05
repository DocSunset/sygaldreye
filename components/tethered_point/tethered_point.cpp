#include "tethered_point.hpp"
#include <cmath>

Eigen::Vector3f tethered_point(Eigen::Vector3f anchor,
                                Eigen::Vector3f desired,
                                float min_dist,
                                float max_dist) {
    Eigen::Vector3f offset = desired - anchor;
    float distance = offset.norm();

    // Degenerate case: desired == anchor
    if (distance < 1e-7f) {
        return anchor + min_dist * Eigen::Vector3f::UnitX();
    }

    // Within range: return desired unchanged
    if (distance >= min_dist && distance <= max_dist) {
        return desired;
    }

    // Too close: push out to min_dist
    if (distance < min_dist) {
        Eigen::Vector3f direction = offset / distance;
        return anchor + min_dist * direction;
    }

    // Too far: pull in to max_dist
    Eigen::Vector3f direction = offset / distance;
    return anchor + max_dist * direction;
}
