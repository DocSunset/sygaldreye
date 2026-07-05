#pragma once
#include <Eigen/Core>
#include <vector>

struct SphereVertex {
    Eigen::Vector3f position;
    Eigen::Vector3f normal;
};

struct SphereGeometry {
    std::vector<SphereVertex> vertices;
    std::vector<uint16_t>     indices;
};

SphereGeometry make_sphere(int lon_slices, int lat_slices);
