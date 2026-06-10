// Copyright 2025 Travis West
// Sky dome snapshot: animated 24-second day/night cycle.
// Usage: sky_dome_snapshot [out.mp4] [duration_s] [fps]
#include "host_gl_context.hpp"
#include "scene_snapshot.hpp"
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

int main(int argc, char** argv) {
    const char* out        = argc > 1 ? argv[1] : "/tmp/sky_dome_snapshot.mp4";
    float       duration_s = argc > 2 ? std::stof(argv[2]) : 24.0f;
    int         fps        = argc > 3 ? std::stoi(argv[3]) : 30;

    auto ctx = HostGlContext::create();
    if (!ctx) { std::fprintf(stderr, "Failed to create GL context\n"); return 1; }

    SkyParams sp;
    sp.body_angular_radius = 0.012f; // slightly larger sun disc
    SkyDome sky = SkyDome::create(sp);

    constexpr int W = 1280, H = 720;
    Eigen::Matrix4f proj = perspective(70.0f * std::numbers::pi_v<float> / 180.0f,
                                       static_cast<float>(W) / static_cast<float>(H),
                                       0.1f, 2000.0f);
    // Fixed view: look slightly above the horizon toward east-southeast
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();
    Eigen::Vector3f fwd = Eigen::Vector3f{0.8f, 0.15f, -0.6f}.normalized();
    Eigen::Vector3f s   = fwd.cross(Eigen::Vector3f{0,1,0}).normalized();
    Eigen::Vector3f u   = s.cross(fwd);
    view.row(0) << s.x(),   s.y(),   s.z(),   0.0f;
    view.row(1) << u.x(),   u.y(),   u.z(),   0.0f;
    view.row(2) <<-fwd.x(),-fwd.y(),-fwd.z(), 0.0f;

    SnapshotParams params;
    params.width      = W;
    params.height     = H;
    params.projection = proj;
    params.view       = view;

    // Full day/night cycle: 24s total.
    // Elevation: sin wave shifted so it's negative at t=0 (midnight).
    // Azimuth sweeps east (x>0) to west (x<0) over the daytime half.
    auto sun_at = [&](float t) {
        // phase: 0..2pi over duration
        float phase   = t / duration_s * 2.0f * std::numbers::pi_v<float>;
        // elevation: -0.35 at midnight, peaks at +0.45 at noon
        float elev    = std::sin(phase - std::numbers::pi_v<float> * 0.5f) * 0.45f;
        // Azimuth: east at sunrise, overhead at noon, west at sunset
        // Use half the phase cycle to sweep 180 degrees
        float azimuth = phase * 0.5f; // 0..pi over full duration
        float horiz   = std::cos(elev); // horizontal length of body direction
        Eigen::Vector3f body_dir{
            std::cos(azimuth) * horiz, // x: east positive
            elev,                       // y: positive above horizon
            -std::sin(azimuth) * horiz  // z: negative into screen
        };
        return std::make_pair(elev, body_dir.normalized());
    };

    auto sun_color_at = [](float elev) -> Eigen::Vector4f {
        float day  = std::clamp(elev * 3.0f, 0.0f, 1.0f);
        float dusk = std::clamp(1.0f - std::abs(elev) * 4.0f, 0.0f, 1.0f);
        dusk = dusk * dusk;
        float r = 1.0f;
        float g = day * 0.98f + dusk * 0.50f;
        float b = day * 0.85f + dusk * 0.10f;
        return {r, g, b, 1.0f};
    };

    auto draw = [&](Eigen::Matrix4f const& p, Eigen::Matrix4f const& v, float t) {
        auto [elev, body_dir] = sun_at(t);

        SkyParams sp2     = sp;
        sp2.sun_elevation = elev;
        sp2.body_dir      = body_dir;
        sp2.body_color    = sun_color_at(elev);
        sky.set_params(sp2);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        sky.draw(p * v);
    };

    bool ok = write_animation(params, duration_s, fps, draw, out);
    if (!ok) { std::fprintf(stderr, "Failed to write %s\n", out); return 1; }
    std::printf("Wrote %s\n", out);
    return 0;
}
