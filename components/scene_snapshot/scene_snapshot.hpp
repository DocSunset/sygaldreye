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

bool write_snapshot(SnapshotParams const&,
                    std::function<void(Eigen::Matrix4f const& proj,
                                       Eigen::Matrix4f const& view)> const& draw_fn,
                    std::string const& png_path);
