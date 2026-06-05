#pragma once
#include <array>
#include <Eigen/Core>
#include "tri_mesh.hpp"

struct TerrainParams {
    int   grid_w         = 64;
    int   grid_h         = 64;
    float cell_size      = 1.0f;
    float height_scale   = 20.0f;
    int   octaves        = 5;
    float lacunarity     = 2.0f;
    float gain           = 0.5f;
    float noise_offset_x = 0.0f;
    float noise_offset_z = 0.0f;
    std::array<Eigen::Vector4f, 4> band_colors;
};

TriMeshData generate_terrain(TerrainParams const&);
