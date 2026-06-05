#pragma once
#include <array>
#include <Eigen/Core>
#include "tri_mesh.hpp"
#include "light.hpp"
#include "material.hpp"
#include "gl_program.hpp"
#include <memory>

struct TerrainBand {
    float           threshold; // normalized height [0,1]
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
    // Semantic color bands (sorted by threshold ascending)
    std::array<TerrainBand, 5> bands = {{
        {0.18f, {0.08f, 0.18f, 0.38f}},  // deep water
        {0.28f, {0.76f, 0.70f, 0.50f}},  // sand
        {0.55f, {0.25f, 0.52f, 0.18f}},  // grass
        {0.75f, {0.50f, 0.44f, 0.38f}},  // rock
        {1.00f, {0.95f, 0.95f, 1.00f}},  // snow
    }};
    Light    sun      = {LightType::Directional,
                         {-0.4f, -0.9f, -0.2f},
                         {1.0f, 0.95f, 0.85f}, 1.2f};
    Material material = {{0.06f, 0.08f, 0.10f},
                         {0.80f, 0.80f, 0.80f},
                         {0.15f, 0.15f, 0.15f}, 16.0f};
};

TriMeshData generate_terrain(TerrainParams const&);

class TerrainRenderer {
public:
    static TerrainRenderer create(TerrainParams const&);

    TerrainRenderer() = default;
    ~TerrainRenderer() = default;
    TerrainRenderer(TerrainRenderer const&) = delete;
    TerrainRenderer& operator=(TerrainRenderer const&) = delete;
    TerrainRenderer(TerrainRenderer&&) noexcept = default;
    TerrainRenderer& operator=(TerrainRenderer&&) noexcept = default;

    void set_sun(Light const& sun);
    void draw(Eigen::Matrix4f const& mvp,
              Eigen::Matrix4f const& model,
              Eigen::Vector3f const& view_pos) const;

private:
    TerrainParams              params_;
    TriMesh                    mesh_;
    std::unique_ptr<GlProgram> prog_;
    GLint mvp_loc_      = -1;
    GLint model_loc_    = -1;
    GLint view_pos_loc_ = -1;
    GLint sun_dir_loc_  = -1;
    GLint sun_col_loc_  = -1;
    GLint sun_int_loc_  = -1;
    GLint mat_amb_loc_  = -1;
    GLint mat_dif_loc_  = -1;
    GLint mat_spe_loc_  = -1;
    GLint mat_shi_loc_  = -1;
};
