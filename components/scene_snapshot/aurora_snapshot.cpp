// Copyright 2025 Travis West
#include "host_gl_context.hpp"
#include "scene_snapshot.hpp"
#include "aurora.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Geometry>
#include <cmath>
#include <cstdio>

static Eigen::Matrix4f perspective(float fovy_rad, float aspect, float near_z, float far_z) {
    float f = 1.0f / std::tan(fovy_rad * 0.5f);
    Eigen::Matrix4f m = Eigen::Matrix4f::Zero();
    m(0,0) = f / aspect;
    m(1,1) = f;
    m(2,2) = (far_z + near_z) / (near_z - far_z);
    m(2,3) = (2.0f * far_z * near_z) / (near_z - far_z);
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
    const char* out        = argc > 1 ? argv[1] : "/tmp/aurora.mp4";
    float       duration_s = argc > 2 ? std::stof(argv[2]) : 12.0f;
    int         fps        = argc > 3 ? std::stoi(argv[3]) : 30;

    auto ctx = HostGlContext::create();
    if (!ctx) { std::fprintf(stderr, "Failed to create GL context\n"); return 1; }

    Aurora aurora = Aurora::create({});

    constexpr int W = 1280, H = 720;
    float fov = 80.0f * float(M_PI) / 180.0f;
    Eigen::Matrix4f proj = perspective(fov, float(W) / float(H), 1.0f, 1000.0f);
    // Camera on the ground looking up into the aurora curtains overhead
    Eigen::Matrix4f view = lookat(
        {0.f,   5.f,  0.f},
        {0.f, 155.f, 60.f},
        {0.f,   1.f,  0.f}
    );

    SnapshotParams params;
    params.width      = W;
    params.height     = H;
    params.projection = proj;
    params.view       = view;
    params.time_s     = 0.0f;

    auto draw = [&](Eigen::Matrix4f const& p, Eigen::Matrix4f const& v, float t) {
        aurora.update(t);
        glClearColor(0.01f, 0.01f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        aurora.draw(p * v);
    };

    bool ok = write_animation(params, duration_s, fps, draw, out);
    if (!ok) { std::fprintf(stderr, "Failed to write %s\n", out); return 1; }
    std::printf("Wrote %s\n", out);
    return 0;
}
