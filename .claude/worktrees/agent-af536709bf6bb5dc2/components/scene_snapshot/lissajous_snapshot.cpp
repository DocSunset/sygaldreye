// Copyright 2025 Travis West
#include "host_gl_context.hpp"
#include "scene_snapshot.hpp"
#include "lissajous.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Core>
#include <cstdio>
#include <cstdlib>

// Usage: lissajous_snapshot [out.mp4] [duration_s] [fps]
int main(int argc, char** argv) {
    const char* out        = argc > 1 ? argv[1] : "/tmp/lissajous.mp4";
    float       duration_s = argc > 2 ? std::stof(argv[2]) : 16.0f;
    int         fps        = argc > 3 ? std::stoi(argv[3]) : 30;

    auto ctx = HostGlContext::create();
    if (!ctx) { std::fprintf(stderr, "Failed to create GL context\n"); return 1; }

    Lissajous vis = Lissajous::create({});

    constexpr int W = 1280, H = 720;
    // Orthographic: [-1.5, 1.5] x [-1.2, 1.2]
    Eigen::Matrix4f proj = Eigen::Matrix4f::Zero();
    float rx = 1.5f, ry = rx * float(H) / float(W);
    proj(0,0) = 1.0f / rx;
    proj(1,1) = 1.0f / ry;
    proj(2,2) = -1.0f / 10.0f;
    proj(3,3) = 1.0f;
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    SnapshotParams params;
    params.width      = W;
    params.height     = H;
    params.projection = proj;
    params.view       = view;
    params.time_s     = 0.0f;

    auto draw = [&](Eigen::Matrix4f const& p, Eigen::Matrix4f const& v, float t) {
        vis.update(t);
        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        vis.draw(p * v);
    };

    bool ok = write_animation(params, duration_s, fps, draw, out);
    if (!ok) { std::fprintf(stderr, "Failed to write %s\n", out); return 1; }
    std::printf("Wrote %s\n", out);
    return 0;
}
