#pragma once
#include <Eigen/Core>
#include <functional>
#include <string>

struct SnapshotParams {
    int width  = 1280;
    int height = 720;
    Eigen::Matrix4f projection;
    Eigen::Matrix4f view;
    float time_s = 0.0f;
};

using SnapshotDrawFn     = std::function<void(Eigen::Matrix4f const& proj, Eigen::Matrix4f const& view)>;
using SnapshotDrawFnTime = std::function<void(Eigen::Matrix4f const& proj, Eigen::Matrix4f const& view, float time_s)>;

bool write_snapshot(SnapshotParams const&, SnapshotDrawFn const& draw_fn, std::string const& png_path);

bool write_animation(SnapshotParams const& base, float duration_s, int fps,
                     SnapshotDrawFnTime const& draw_fn, std::string const& mp4_path);
