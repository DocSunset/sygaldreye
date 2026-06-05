#pragma once
#include <Eigen/Core>
#include <array>
#include <functional>
#include <optional>
#include <span>
#include <string>

#include "cylinder_mesh.hpp"
#include "grab_target.hpp"
#include "rgba_shader.hpp"
#include "sphere_mesh.hpp"
#include "text_mesh.hpp"

struct HandState;

struct RubberBandController {
    static RubberBandController create(
        Eigen::Vector3f anchor_pos,
        std::function<std::string(Eigen::Vector3f)> label_fn = {});

    void update(std::span<const HandState> hands);
    void draw(const Eigen::Matrix4f& view_proj) const;

    [[nodiscard]] Eigen::Vector3f offset() const;

    ~RubberBandController();
    RubberBandController(const RubberBandController&)              = delete;
    RubberBandController& operator=(const RubberBandController&)   = delete;
    RubberBandController(RubberBandController&&) noexcept          = default;
    RubberBandController& operator=(RubberBandController&&) noexcept = default;

private:
    RubberBandController() = default;

    std::array<GrabTarget, 2>                    targets_{};
    std::optional<SphereMesh>                    sphere_mesh_{};
    std::optional<CylinderMesh>                  cylinder_mesh_{};
    RgbaShader                                   rgba_shader_{};
    TextMesh                                     text_mesh_{};
    std::function<std::string(Eigen::Vector3f)>  label_fn_{};
};
