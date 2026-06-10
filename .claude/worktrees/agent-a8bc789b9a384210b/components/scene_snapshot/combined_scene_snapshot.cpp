// Copyright 2025 Travis West
#include "host_gl_context.hpp"
#include "scene_snapshot.hpp"
#include "water_surface.hpp"
#include "sky_dome.hpp"
#include "terrain_generator.hpp"
#include "tree_generator.hpp"
#include "tri_mesh.hpp"
#include "billboard_quad.hpp"
#include "lit_shader.hpp"
#include "light.hpp"
#include "material.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Geometry>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static Eigen::Matrix4f perspective(float fovy_rad, float aspect, float znear, float zfar) {
    float f = 1.0f / std::tan(fovy_rad * 0.5f);
    Eigen::Matrix4f m = Eigen::Matrix4f::Zero();
    m(0,0) = f / aspect;
    m(1,1) = f;
    m(2,2) = (zfar + znear) / (znear - zfar);
    m(2,3) = (2.0f * zfar * znear) / (znear - zfar);
    m(3,2) = -1.0f;
    return m;
}

static Eigen::Matrix4f lookat(Eigen::Vector3f eye, Eigen::Vector3f center, Eigen::Vector3f up) {
    Eigen::Vector3f f = (center - eye).normalized();
    Eigen::Vector3f s = f.cross(up).normalized();
    Eigen::Vector3f u = s.cross(f);
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    m.row(0) << s.x(), s.y(), s.z(), -s.dot(eye);
    m.row(1) << u.x(), u.y(), u.z(), -u.dot(eye);
    m.row(2) <<-f.x(),-f.y(),-f.z(),  f.dot(eye);
    return m;
}

// Direction FROM sun TO scene, given elevation and azimuth in radians
static Eigen::Vector3f sun_direction(float elevation, float azimuth) {
    float ce = std::cos(elevation);
    return Eigen::Vector3f{
        -ce * std::cos(azimuth),
        -std::sin(elevation),
        -ce * std::sin(azimuth)
    }.normalized();
}



int main(int argc, char** argv) {
    const char* out        = argc > 1 ? argv[1] : "/tmp/combined_scene.mp4";
    float       start_t    = argc > 2 ? std::stof(argv[2]) : 0.0f;
    float       duration_s = argc > 3 ? std::stof(argv[3]) : 0.0f;
    int         fps        = argc > 4 ? std::stoi(argv[4]) : 30;

    auto ctx = HostGlContext::create();
    if (!ctx) { fprintf(stderr, "Failed to create GL context\n"); return 1; }

    // --- Terrain ---
    constexpr int   TERRAIN_W    = 128;
    constexpr int   TERRAIN_H    = 128;
    constexpr float CELL_SIZE    = 1.5f;
    constexpr float TERRAIN_SPAN = (TERRAIN_W - 1) * CELL_SIZE;

    TerrainParams tp;
    tp.grid_w       = TERRAIN_W;
    tp.grid_h       = TERRAIN_H;
    tp.cell_size    = CELL_SIZE;
    tp.height_scale = 25.0f;
    tp.octaves      = 6;
    // Use TerrainParams default bands (deep water, sand, grass, rock, snow)

    TriMeshData terrain_data = generate_terrain(tp);

    // Center terrain at world origin
    float const ox = TERRAIN_SPAN * 0.5f;
    float const oz = TERRAIN_SPAN * 0.5f;
    for (auto& v : terrain_data.vertices) {
        v.position.x() -= ox;
        v.position.z() -= oz;
    }

    TriMesh terrain_mesh = TriMesh::create(terrain_data);

    LitShader lit;
    lit.init();

    Material terrain_mat;
    terrain_mat.ambient   = {0.15f, 0.18f, 0.12f};
    terrain_mat.diffuse   = {0.70f, 0.70f, 0.65f};
    terrain_mat.specular  = {0.05f, 0.05f, 0.05f};
    terrain_mat.shininess = 8.0f;

    // --- Water ---
    WaterParams wp;
    WaterSurface water = WaterSurface::create(wp);
    Eigen::Matrix4f water_model = Eigen::Matrix4f::Identity();
    water_model(0,3) = -(wp.grid_w - 1) * wp.cell_size * 0.5f;
    water_model(1,3) = -3.0f;
    water_model(2,3) = -(wp.grid_h - 1) * wp.cell_size * 0.5f;

    // --- Sky ---
    SkyParams sky_p;
    sky_p.radius      = 800.0f;
    sky_p.star_count  = 2000;
    SkyDome sky = SkyDome::create(sky_p);

    // --- Trees ---
    static const float tree_xz[][2] = {
        { -40.0f,  30.0f }, {  55.0f,  20.0f }, { -70.0f, -35.0f },
        {  35.0f, -55.0f }, {  80.0f,  60.0f }, { -80.0f,  65.0f },
        {  10.0f,  75.0f }, { -25.0f, -70.0f }, {  65.0f, -30.0f },
        { -55.0f,  10.0f }, {  25.0f,  50.0f }, { -15.0f,  85.0f },
        {  45.0f, -75.0f }, { -85.0f, -55.0f }, {  70.0f,  -5.0f },
        { -30.0f,  55.0f }, {   5.0f, -45.0f }, {  85.0f,  35.0f },
        { -60.0f,  80.0f }, {  50.0f,  85.0f },
    };
    constexpr int NUM_TREES = 20;

    struct TreeRenderable {
        TriMesh       branches;
        BillboardQuad leaves;
        Eigen::Matrix4f model;
    };

    Material branch_mat;
    branch_mat.ambient   = {0.10f, 0.07f, 0.04f};
    branch_mat.diffuse   = {0.45f, 0.30f, 0.15f};
    branch_mat.specular  = {0.02f, 0.02f, 0.02f};
    branch_mat.shininess = 4.0f;

    std::vector<TreeRenderable> trees;
    trees.reserve(NUM_TREES);
    for (int i = 0; i < NUM_TREES; ++i) {
        TreeParams treep;
        treep.branch.seed = static_cast<uint32_t>(42 + i * 7);
        TreeMesh tm = generate_tree(treep);

        TreeRenderable tr;
        tr.branches = TriMesh::create(tm.branches);
        tr.leaves   = BillboardQuad::create(static_cast<int>(tm.leaves.size()));
        tr.leaves.set_instances(tm.leaves);

        tr.model = Eigen::Matrix4f::Identity();
        tr.model(0,3) = tree_xz[i][0];
        tr.model(1,3) = 4.0f;
        tr.model(2,3) = tree_xz[i][1];

        trees.push_back(std::move(tr));
    }

    // --- Camera ---
    constexpr int W = 1280, H = 720;
    Eigen::Vector3f eye    = {0.0f, 80.0f, -120.0f};
    Eigen::Vector3f target = {0.0f,  0.0f,   60.0f};

    Eigen::Matrix4f proj = perspective(50.0f * float(M_PI) / 180.0f,
                                       float(W) / float(H), 0.5f, 2000.0f);
    Eigen::Matrix4f view = lookat(eye, target, {0.0f, 1.0f, 0.0f});

    Eigen::Vector3f cam_right = {view(0,0), view(0,1), view(0,2)};
    Eigen::Vector3f cam_up    = {view(1,0), view(1,1), view(1,2)};

    float const anim_dur = (duration_s > 0.0f) ? duration_s : 20.0f;

    auto draw = [&](Eigen::Matrix4f const& p, Eigen::Matrix4f const& v, float t) {
        float nt = std::clamp((t - start_t) / anim_dur, 0.0f, 1.0f);

        // Sun arc: -20 deg at t=0, peaks at t=0.5, -20 deg at t=1
        float elev_deg  = -20.0f + 110.0f * std::sin(float(M_PI) * nt);
        float elevation = elev_deg * float(M_PI) / 180.0f;
        float azimuth   = float(M_PI) * nt; // east to west

        Eigen::Vector3f sun_dir = sun_direction(elevation, azimuth);
        float sun_intensity = std::max(0.05f, std::sin(elevation) * 1.3f);

        // Warm tint near horizon
        float warm = std::max(0.0f, 1.0f - std::abs(nt - 0.5f) * 4.0f);
        Eigen::Vector3f sun_color = {1.0f,
                                     0.85f + 0.15f * (1.0f - warm),
                                     0.65f + 0.35f * (1.0f - warm)};

        Light sun;
        sun.direction = sun_dir;
        sun.color     = sun_color;
        sun.intensity = sun_intensity;

        // Sky — use procedural atmospheric coloring via sun_elevation
        SkyParams sp           = sky_p;
        sp.sun_elevation       = elevation;
        sp.body_dir            = -sun_dir;
        sp.body_color          = {sun_color.x(), sun_color.y(), sun_color.z(), 1.0f};
        sp.body_angular_radius = 0.014f;
        sky.set_params(sp);

        Eigen::Matrix4f mvp = p * v;

        float day_t = std::clamp(elevation + 0.5f, 0.0f, 1.0f);
        glClearColor(0.01f + 0.54f * day_t, 0.01f + 0.74f * day_t, 0.05f + 0.93f * day_t, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Sky dome (no depth write)
        glDepthMask(GL_FALSE);
        sky.draw(mvp);
        glDepthMask(GL_TRUE);

        Light lights[1] = {sun};

        // Terrain
        lit.use();
        lit.set_mvp(mvp);
        lit.set_model(Eigen::Matrix4f::Identity());
        lit.set_view_pos(eye);
        lit.set_lights(lights);
        lit.set_material(terrain_mat);
        terrain_mesh.draw();

        // Trees
        for (auto& tr : trees) {
            lit.use();
            lit.set_mvp(mvp * tr.model);
            lit.set_model(tr.model);
            lit.set_view_pos(eye);
            lit.set_lights(lights);
            lit.set_material(branch_mat);
            tr.branches.draw();
            tr.leaves.draw(mvp, cam_right, cam_up);
        }

        // Water
        water.update(t);
        water.draw(mvp * water_model, water_model, eye);
    };

    SnapshotParams params;
    params.width      = W;
    params.height     = H;
    params.projection = proj;
    params.view       = view;
    params.time_s     = start_t;

    bool ok;
    if (duration_s > 0.0f) {
        ok = write_animation(params, duration_s, fps, draw, out);
    } else {
        ok = write_snapshot(params, [&](Eigen::Matrix4f const& pp, Eigen::Matrix4f const& vv) {
            draw(pp, vv, start_t);
        }, out);
    }

    if (!ok) { fprintf(stderr, "Failed to write %s\n", out); return 1; }
    printf("Wrote %s\n", out);
    return 0;
}
