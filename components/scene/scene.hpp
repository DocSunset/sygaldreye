#pragma once
#include "cube_instance.hpp"
#include <span>
#include <vector>

struct Scene {
    void update(double time);
    [[nodiscard]] std::span<const CubeInstance> cubes() const;
    void set_controller_poses(const Eigen::Matrix4f* left_model, bool left_valid,
                              const Eigen::Matrix4f* right_model, bool right_valid);

private:
    std::vector<CubeInstance> cubes_;
};
