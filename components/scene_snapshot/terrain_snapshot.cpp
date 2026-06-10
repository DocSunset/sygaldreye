// Copyright 2025 Travis West
// Terrain snapshot: animated sun arc over a procedural terrain.
// Usage: terrain_snapshot [out.mp4] [duration_s] [fps]
#include "host_gl_context.hpp"
#include "scene_snapshot.hpp"
#include "terrain_generator.hpp"
#include "sky_dome.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Geometry>
#include <cmath>
#include <cstdio>
#include <numbers>

static Eigen::Matrix4f perspective(float fovy_rad, float aspect, float near, float far) {
    float f = 1.0f / std::tan(fovy_rad * 0.5f);
    Eigen::Matrix4f m = Eigen::Matrix4f::Zero();
    m(0,0) = f / aspect;
    m(1,1) = f;
    m(2,2) = (far + near) / (near - far);
    m(2,3) = (2.0f * far * near) / (near - far);
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

int main(int argc, char** argv) {
    const char* out        = argc > 1 ? argv[1] : "/tmp/terrain_snapshot.mp4";
    float       duration_s = argc > 2 ? std::stof(argv[2]) : 12.0f;
    int         fps        = argc > 3 ? std::stoi(argv[3]) : 30;

    auto ctx = HostGlContext::create();
    if (!ctx) { std::fprintf(stderr, "Failed to create GL context\n"); return 1; }

    TerrainParams tp;
    tp.grid_w       = 64;
    tp.grid_h       = 64;
    tp.cell_size    = 2.0f;
    tp.height_scale = 20.0f;
    tp.octaves      = 6;
    // Enough ambient to see terrain even in shadow
    tp.material.ambient  = {0.35f, 0.38f, 0.42f};
    tp.material.diffuse  = {0.85f, 0.85f, 0.80f};
    tp.material.specular = {0.10f, 0.10f, 0.10f};
    tp.material.shininess = 12.0f;
    TerrainRenderer terrain = TerrainRenderer::create(tp);

    SkyParams sp;
    SkyDome sky = SkyDome::create(sp);

    constexpr int W = 1280, H = 720;
    Eigen::Matrix4f proj = perspective(50.0f * std::numbers::pi_v<float> / 180.0f,
                                       static_cast<float>(W) / static_cast<float>(H),
                                       0.5f, 2000.0f);
    // Camera: above-and-to-side, looking at terrain center
    float terrain_cx = tp.grid_w * tp.cell_size * 0.5f;
    float terrain_cz = tp.grid_h * tp.cell_size * 0.5f;
    Eigen::Vector3f eye    = {terrain_cx - 30.0f, 55.0f, terrain_cz - 50.0f};
    Eigen::Vector3f center = {terrain_cx, 5.0f,  terrain_cz};
    Eigen::Matrix4f view   = lookat(eye, center, {0.0f, 1.0f, 0.0f});

    SnapshotParams params;
    params.width      = W;
    params.height     = H;
    params.projection = proj;
    params.view       = view;

    // Sun arc: sweeps from east to west over the terrain (half arc, always above horizon)
    // sun direction = direction *from* scene *toward* sun (light source dir is negative of this)
    // In shader: lightDir = normalize(-sun.direction), so sun.direction points away from terrain
    auto sun_dir_at = [&](float t) -> Eigen::Vector3f {
        // Azimuth: 0..pi (east to west)
        float azimuth = t / duration_s * std::numbers::pi_v<float>;
        // Elevation oscillates: sin wave peaking at midday
        float elev_rad = std::numbers::pi_v<float> / 5.0f // ~36 deg max
                       * std::sin(t / duration_s * std::numbers::pi_v<float>);
        // Light direction: pointing from sky down to terrain
        // Positive y = sun above horizon, negative y = below
        return Eigen::Vector3f{
            std::cos(azimuth) * std::cos(elev_rad),
            -std::sin(elev_rad),  // negative y = sun shines downward
            std::sin(azimuth) * std::cos(elev_rad)
        }.normalized();
    };

    auto sun_color_at = [](float elevation) -> Eigen::Vector3f {
        // Warm at dawn/dusk, white at midday
        float day  = std::clamp(elevation * 3.0f, 0.0f, 1.0f);
        float dusk = std::clamp(1.0f - std::abs(elevation) * 4.0f, 0.0f, 1.0f);
        dusk = dusk * dusk;
        Eigen::Vector3f white{1.0f, 0.98f, 0.90f};
        Eigen::Vector3f orange{1.0f, 0.50f, 0.10f};
        return white * day + orange * dusk * (1.0f - day * 0.5f);
    };

    auto draw = [&](Eigen::Matrix4f const& p, Eigen::Matrix4f const& v, float t) {
        // sd: light direction (points from sky toward terrain; y < 0 when sun above horizon)
        Eigen::Vector3f sd  = sun_dir_at(t);
        float sun_elevation = std::clamp(-sd.y(), -1.0f, 1.0f);
        Eigen::Vector3f scol = sun_color_at(sun_elevation);

        SkyParams sp2 = sp;
        sp2.body_dir      = -sd; // points toward sun disc
        sp2.sun_elevation = sun_elevation;
        sp2.body_color    = {scol.x(), scol.y(), scol.z(), 1.0f};
        sky.set_params(sp2);

        Light sun;
        sun.direction = sd;
        sun.color     = scol;
        sun.intensity = std::clamp(sun_elevation * 1.4f + 0.15f, 0.05f, 1.3f);
        terrain.set_sun(sun);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // Draw sky first (depth test disabled inside)
        sky.draw(p * v);

        // Draw terrain
        Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
        terrain.draw(p * v, model, eye);
    };

    bool ok = write_animation(params, duration_s, fps, draw, out);
    if (!ok) { std::fprintf(stderr, "Failed to write %s\n", out); return 1; }
    std::printf("Wrote %s\n", out);
    return 0;
}
