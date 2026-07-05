// Copyright 2025 Travis West
#include "sphere_geometry.hpp"
#include <cmath>
#include <numbers>

SphereGeometry make_sphere(int lon_slices, int lat_slices) {
    SphereGeometry geo;

    for (int lat = 0; lat <= lat_slices; ++lat) {
        float phi = std::numbers::pi_v<float> * static_cast<float>(lat) / static_cast<float>(lat_slices);
        float sin_phi = std::sin(phi);
        float cos_phi = std::cos(phi);
        for (int lon = 0; lon <= lon_slices; ++lon) {
            float theta = 2.0f * std::numbers::pi_v<float> * static_cast<float>(lon) / static_cast<float>(lon_slices);
            Eigen::Vector3f n{std::cos(theta) * sin_phi, cos_phi, std::sin(theta) * sin_phi};
            geo.vertices.push_back({n, n});
        }
    }

    for (int lat = 0; lat < lat_slices; ++lat) {
        for (int lon = 0; lon < lon_slices; ++lon) {
            uint16_t a = static_cast<uint16_t>(lat * (lon_slices + 1) + lon);
            uint16_t b = static_cast<uint16_t>(a + 1);
            uint16_t c = static_cast<uint16_t>(a + (lon_slices + 1));
            uint16_t d = static_cast<uint16_t>(c + 1);
            geo.indices.insert(geo.indices.end(), {a, c, b, b, c, d});
        }
    }

    return geo;
}
