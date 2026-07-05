// Copyright 2025 Travis West
#pragma once
#include <array>
#include <memory>
#include <string_view>

#include <Eigen/Core>

#include "render_payloads.hpp"  // MeshPtr
#include "sygaldry_endpoints.hpp"
#include "tri_mesh.hpp"

struct TerrainBand {
    float           threshold;  // normalized height [0,1]
    Eigen::Vector3f color;
};

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
    // Semantic color bands (sorted by threshold ascending), baked per vertex.
    std::array<TerrainBand, 5> bands = {{
        {0.18f, {0.08f, 0.18f, 0.38f}},  // deep water
        {0.28f, {0.76f, 0.70f, 0.50f}},  // sand
        {0.55f, {0.25f, 0.52f, 0.18f}},  // grass
        {0.75f, {0.50f, 0.44f, 0.38f}},  // rock
        {1.00f, {0.95f, 0.95f, 1.00f}},  // snow
    }};
};

TriMeshData generate_terrain(TerrainParams const&);

// terrain: a geometry generator. Builds a heightmap mesh with per-vertex band
// colors + face normals and emits it as a MeshPtr. Lighting/drawing belong to
// vertex_color_mesh + draw downstream — GL left this node (render-as-nodes).
class Terrain {
   public:
    static consteval std::string_view name() { return "terrain"; }
    static consteval std::string_view source_header() {
        return "components/terrain_generator/terrain_generator.hpp";
    }
    static consteval std::string_view source_cpp() {
        return "components/terrain_generator/terrain_generator.cpp";
    }

    struct endpoints {
        normalled_in<float, fp(0.f), fp(100.f), fp(20.f)> height_scale;
        normalled_in<float, fp(1.f), fp(4.f), fp(2.f)> lacunarity;
        normalled_in<float, fp(0.1f), fp(1.f), fp(0.5f)> gain;
        normalled_in<float, fp(-100.f), fp(100.f), fp(0.f)> noise_offset_x;
        normalled_in<float, fp(-100.f), fp(100.f), fp(0.f)> noise_offset_z;
        ::out<MeshPtr> mesh;
    } endpoints;

    void operator()(double time_s);

   private:
    TerrainParams params_;
    MeshPtr       mesh_;
    bool          generated_ = false;
};
