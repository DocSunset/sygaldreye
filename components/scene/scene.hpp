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
    void set_controller_poses();

private:
    std::vector<CubeInstance> cubes_;
};
