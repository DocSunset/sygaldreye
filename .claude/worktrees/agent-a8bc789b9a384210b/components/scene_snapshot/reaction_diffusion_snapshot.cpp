// Copyright 2025 Travis West
#include "host_gl_context.hpp"
#include "scene_snapshot.hpp"
#include "reaction_diffusion.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Geometry>
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Orthographic projection: maps [-scale, scale] to NDC
static Eigen::Matrix4f ortho(float scale, float aspect) {
    Eigen::Matrix4f m = Eigen::Matrix4f::Zero();
    m(0,0) =  1.0f / (scale * aspect);
    m(1,1) =  1.0f / scale;
    m(2,2) = -1.0f;          // depth range [-1, 1]
    m(3,3) =  1.0f;
    return m;
}

// Top-down view: camera at (0, 2, 0) looking down at XZ plane
static Eigen::Matrix4f top_down_view() {
    Eigen::Matrix4f m = Eigen::Matrix4f::Identity();
    // Rotate -90° around X: +Y world becomes -Z view
    m(1,1) =  0.0f; m(1,2) =  1.0f;
    m(2,1) = -1.0f; m(2,2) =  0.0f;
    return m;
}

// Usage: reaction_diffusion_snapshot <out.png|out.mp4> [duration_s] [fps] [warmup_steps]
int main(int argc, char** argv) {
    const char* out          = argc > 1 ? argv[1] : "/tmp/reaction_diffusion.mp4";
    float       duration_s   = argc > 2 ? std::stof(argv[2]) : 15.0f;
    int         fps          = argc > 3 ? std::stoi(argv[3]) : 30;
    int         warmup_steps = argc > 4 ? std::stoi(argv[4]) : 2000;

    auto ctx = HostGlContext::create();
    if (!ctx) { fprintf(stderr, "Failed to create GL context\n"); return 1; }

    RDParams params;
    // Coral: Du=0.16, Dv=0.08, F=0.060, k=0.062
    // Use slightly more steps per frame for richer pattern at snapshot time
    params.steps_per_frame = 4;

    ReactionDiffusion rd = ReactionDiffusion::create(params);

    // Warm up the simulation to develop interesting patterns
    fprintf(stdout, "Warming up simulation (%d steps)...\n", warmup_steps);
    fflush(stdout);
    // Run warmup in batches matching steps_per_frame
    int warmup_calls = warmup_steps / params.steps_per_frame;
    for (int i = 0; i < warmup_calls; ++i)
        rd.update();
    fprintf(stdout, "Warmup complete.\n");
    fflush(stdout);

    constexpr int W = 1280, H = 720;
    float aspect = float(W) / float(H);

    Eigen::Matrix4f proj = ortho(1.0f, aspect);
    Eigen::Matrix4f view = top_down_view();

    SnapshotParams sp;
    sp.width      = W;
    sp.height     = H;
    sp.projection = proj;
    sp.view       = view;
    sp.time_s     = 0.0f;

    auto draw = [&](Eigen::Matrix4f const& p, Eigen::Matrix4f const& v, float /*t*/) {
        rd.update();
        glClearColor(0.05f, 0.15f, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        rd.draw(p * v);
    };

    bool ok = write_animation(sp, duration_s, fps, draw, out);

    if (!ok) { fprintf(stderr, "Failed to write %s\n", out); return 1; }
    printf("Wrote %s\n", out);
    return 0;
}
