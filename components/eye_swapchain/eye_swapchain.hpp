#pragma once
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <array>
#include <span>
#include <vector>

[[nodiscard]] int64_t choose_swapchain_format(std::span<const int64_t> formats);

struct EyeSwapchain {
    EyeSwapchain() = default;
    ~EyeSwapchain();
    EyeSwapchain(const EyeSwapchain&) = delete;
    EyeSwapchain& operator=(const EyeSwapchain&) = delete;
    EyeSwapchain(EyeSwapchain&&) noexcept;
    EyeSwapchain& operator=(EyeSwapchain&&) noexcept;

    [[nodiscard]] GLuint      fbo(uint32_t index) const { return fbos[index]; }
    [[nodiscard]] XrSwapchain xr_handle()         const { return handle; }
    [[nodiscard]] uint32_t    width()              const { return width_; }
    [[nodiscard]] uint32_t    height()             const { return height_; }

    XrSwapchain handle = XR_NULL_HANDLE;
    uint32_t    width_  = 0;
    uint32_t    height_ = 0;
    std::vector<XrSwapchainImageOpenGLESKHR> images;
    std::vector<GLuint> fbos;
    std::vector<GLuint> depth_rbs;
};

bool create_swapchains(XrInstance, XrSystemId, XrSession, std::array<EyeSwapchain, 2>& out);
