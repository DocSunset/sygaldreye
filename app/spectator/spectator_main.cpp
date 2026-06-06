// Copyright 2025 Travis West
// Phase 6 spectator: renders water + sky on Linux using a fixed overview camera.
#include "host_gl_window.hpp"
#include "water_surface.hpp"
#include "sky_dome.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Core>
#include <cmath>
#include <cstdio>
#include <optional>

namespace {

// Simple orthographic-ish overview: camera 20m above origin looking down at 30°.
Eigen::Matrix4f make_view() {
    Eigen::Matrix4f v = Eigen::Matrix4f::Identity();
    // pitch -30°
    float c = std::cos(-0.52f);
    float s = std::sin(-0.52f);
    v(1,1) =  c; v(1,2) = s;
    v(2,1) = -s; v(2,2) = c;
    v(1,3) = -20.0f * c;
    v(2,3) =  20.0f * s;
    return v;
}

Eigen::Matrix4f make_proj(float aspect) {
    float fov  = 0.9f; // ~52°
    float near = 0.1f;
    float far  = 1000.0f;
    float f    = 1.0f / std::tan(fov / 2.0f);
    Eigen::Matrix4f p = Eigen::Matrix4f::Zero();
    p(0,0) = f / aspect;
    p(1,1) = f;
    p(2,2) = (far + near) / (near - far);
    p(2,3) = (2.0f * far * near) / (near - far);
    p(3,2) = -1.0f;
    return p;
}

} // namespace

int main() {
    auto win = HostGlWindow::create(1280, 720, "eyeballs spectator");
    if (!win) {
        std::puts("spectator: could not create window");
        return 1;
    }

    WaterParams wp;
    wp.grid_w    = 64;
    wp.grid_h    = 64;
    wp.cell_size = 0.375f;
    auto water = WaterSurface::create(wp);

    SkyParams sp;
    sp.radius     = 500.0f;
    sp.star_count = 2000;
    auto sky = SkyDome::create(sp);

    const Eigen::Matrix4f view = make_view();
    const Eigen::Vector3f view_pos{0.0f, 20.0f, 0.0f};

    constexpr float kWaterY    = -1.5f;
    constexpr float kCellSize  = 0.375f;
    constexpr int   kGridW     = 64;
    constexpr int   kGridH     = 64;
    Eigen::Matrix4f water_model = Eigen::Matrix4f::Identity();
    water_model(0,3) = -(kGridW - 1) * kCellSize * 0.5f;
    water_model(1,3) = kWaterY;
    water_model(2,3) = -(kGridH - 1) * kCellSize * 0.5f;

    double time_s = 0.0;

    win->run([&](int w, int h) {
        glViewport(0, 0, w, h);
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        float aspect = (h > 0) ? static_cast<float>(w) / static_cast<float>(h) : 1.0f;
        const Eigen::Matrix4f pv = make_proj(aspect) * view;

        glDepthMask(GL_FALSE);
        sky.draw(pv);
        glDepthMask(GL_TRUE);

        water.update(static_cast<float>(time_s));
        water.draw(pv * water_model, water_model, view_pos);

        time_s += 1.0 / 60.0;
    });

    return 0;
}
