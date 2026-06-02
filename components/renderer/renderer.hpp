#pragma once
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <utility>
#include <vector>
#include <array>
#include <span>
#include "scene.hpp"
#include "cube_mesh.hpp"

struct EyeSwapchain {
    XrSwapchain handle = XR_NULL_HANDLE;
    uint32_t    width  = 0;
    uint32_t    height = 0;
    std::vector<XrSwapchainImageOpenGLESKHR> images;
    std::vector<GLuint> fbos;
    std::vector<GLuint> depth_rbs; // one depth renderbuffer per FBO

    EyeSwapchain() = default;
    ~EyeSwapchain();
    EyeSwapchain(const EyeSwapchain&) = delete;
    EyeSwapchain& operator=(const EyeSwapchain&) = delete;
    EyeSwapchain(EyeSwapchain&& other) noexcept;
    EyeSwapchain& operator=(EyeSwapchain&& other) noexcept;

    // returns FBO for the given acquired image index
    GLuint fbo(uint32_t index) const { return fbos[index]; }
};

struct Renderer {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig  config  = nullptr;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;

    std::array<EyeSwapchain, 2> eyes{};

    // Cached last-frame layer (valid until next render_eyes call)
    std::array<XrCompositionLayerProjectionView, 2> projViews{};
    XrCompositionLayerProjection projLayer{};

    Renderer() = default;
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&& other) noexcept;
    Renderer& operator=(Renderer&& other) noexcept;

    bool init();
    XrGraphicsBindingOpenGLESAndroidKHR graphics_binding() const;
    bool create_swapchains(XrInstance, XrSystemId, XrSession);
    // Renders eyes and locates views. Returns true and fills projLayer/projViews if layer is ready.
    bool render_eyes(XrInstance, XrSession, XrSpace refSpace, XrTime predictedDisplayTime,
                     std::span<const CubeInstance> cubes);

private:
    CubeMesh cube_mesh_;
};
