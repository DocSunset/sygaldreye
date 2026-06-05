#include "host_gl_context.hpp"
#include "scene_snapshot.hpp"
#include "water_surface.hpp"
#include <GLES3/gl3.h>
#include <Eigen/Geometry>
#include <cmath>
#include <cstdio>
#include <cstdlib>

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

// Usage: water_surface_snapshot <out.png|out.mp4> [start_t] [duration_s] [fps]
int main(int argc, char** argv) {
    const char* out        = argc > 1 ? argv[1] : "/tmp/water_surface_snapshot.png";
    float       start_t    = argc > 2 ? std::stof(argv[2]) : 0.0f;
    float       duration_s = argc > 3 ? std::stof(argv[3]) : 0.0f;
    int         fps        = argc > 4 ? std::stoi(argv[4]) : 30;

    auto ctx = HostGlContext::create();
    if (!ctx) { fprintf(stderr, "Failed to create GL context\n"); return 1; }

    WaterSurface water = WaterSurface::create({});

    constexpr int W = 1280, H = 720;
    Eigen::Matrix4f proj = perspective(45.0f * float(M_PI) / 180.0f, float(W) / float(H), 0.1f, 400.0f);
    Eigen::Matrix4f view = lookat({31.5f, 35.0f, -25.0f}, {31.5f, 0.0f, 31.5f}, {0.0f, 1.0f, 0.0f});

    SnapshotParams params;
    params.width      = W;
    params.height     = H;
    params.projection = proj;
    params.view       = view;
    params.time_s     = start_t;

    auto draw = [&](Eigen::Matrix4f const& p, Eigen::Matrix4f const& v, float t) {
        water.update(t);
        glClearColor(0.05f, 0.08f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        water.draw(p * v, Eigen::Matrix4f::Identity(), Eigen::Vector3f{31.5f, 35.0f, -25.0f});
    };

    bool ok;
    if (duration_s > 0.0f) {
        ok = write_animation(params, duration_s, fps, draw, out);
    } else {
        water.update(start_t);
        ok = write_snapshot(params, [&](Eigen::Matrix4f const& p, Eigen::Matrix4f const& v) {
            draw(p, v, start_t);
        }, out);
    }

    if (!ok) { fprintf(stderr, "Failed to write %s\n", out); return 1; }
    printf("Wrote %s\n", out);
    return 0;
}
