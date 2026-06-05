#pragma once
#include "cube_instance.hpp"
#include "light.hpp"
#include <openxr/openxr.h>
#include <array>
#include <cstddef>
#include <optional>
#include <span>

struct Scene {
    void update(double time);
    /// Returned span is invalidated by any subsequent call to update() or set_controller_poses().
    [[nodiscard]] std::span<const CubeInstance> cubes() const;
    [[nodiscard]] std::span<const Light> lights() const;
    void set_controller_poses(std::optional<XrPosef> left_pose,
                              std::optional<XrPosef> right_pose);

private:
    CubeInstance world_cube_ = {};
    std::array<std::optional<CubeInstance>, 2> controller_cubes_{};
    mutable std::array<CubeInstance, 3> cubes_cache_{};
    mutable size_t cube_count_ = 0;
    std::array<Light, 1> lights_{};
};
