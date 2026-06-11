#pragma once
#include "egl_context.hpp"
#include "eye_swapchain.hpp"
#include "vr_math.hpp"
#include <array>
#include <functional>
#include <Eigen/Core>

struct Renderer {
    Renderer() = default;
    ~Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) noexcept = default;
    Renderer& operator=(Renderer&&) noexcept = default;

    [[nodiscard]] std::optional<RendererBinding> init();
    bool create_swapchains(XrInstance, XrSystemId, XrSession);
    // Invoked once per frame for eye 0 with the RESOLVED (non-MSAA) eye
    // framebuffer bound for reading — the only safe glReadPixels point.
    std::function<void(int width, int height)> post_resolve_hook;

    bool render_eyes(XrInstance, XrSession, XrSpace refSpace, XrTime predictedDisplayTime,
                     const std::function<void(const Eigen::Matrix4f& proj,
                                              const Eigen::Matrix4f& view)>& on_draw);

    [[nodiscard]] const XrCompositionLayerProjection& proj_layer() const { return projLayer_; }

private:
    EglContext egl_{};
    std::array<EyeSwapchain, 2> eyes_{};

    std::array<XrCompositionLayerProjectionView, 2> projViews_{};
    XrCompositionLayerProjection projLayer_{};

    bool   firstEyeRender_ = true;
    bool   layerLogged_    = false;
    double lastLocateErr_  = 0.0;
};
