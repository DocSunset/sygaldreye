#pragma once
#include <Eigen/Core>
#include <span>
#include <vector>

struct CubeInstance {
    Eigen::Matrix4f model;
};

struct Scene {
    void update(double time);
    std::span<const CubeInstance> cubes() const;
    void set_controller_poses(const Eigen::Matrix4f* left_model, bool left_valid,
                              const Eigen::Matrix4f* right_model, bool right_valid);

private:
    std::vector<CubeInstance> cubes_;
};
