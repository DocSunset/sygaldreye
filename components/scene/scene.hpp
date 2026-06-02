#pragma once
#include "cube_instance.hpp"
#include <optional>
#include <span>
#include <vector>

struct Scene {
    void update(double time);
    [[nodiscard]] std::span<const CubeInstance> cubes() const;
    void set_controller_poses(std::optional<Eigen::Matrix4f> left_model,
                              std::optional<Eigen::Matrix4f> right_model);

private:
    std::vector<CubeInstance> cubes_;
};
