#pragma once
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <memory>
#include <vector>
#include <array>
#include <span>
#include "../scene/cube_instance.hpp"

struct CubeMesh;

struct EyeSwapchain {
    EyeSwapchain() = default;
    ~EyeSwapchain();
    EyeSwapchain(const EyeSwapchain&) = delete;
    EyeSwapchain& operator=(const EyeSwapchain&) = delete;
    EyeSwapchain(EyeSwapchain&& other) noexcept;
    EyeSwapchain& operator=(EyeSwapchain&& other) noexcept;

    // returns FBO for the given acquired image index
    GLuint fbo(uint32_t index) const { return fbos[index]; }

private:
    friend struct Renderer;
    XrSwapchain handle = XR_NULL_HANDLE;
    uint32_t    width  = 0;
    uint32_t    height = 0;
    std::vector<XrSwapchainImageOpenGLESKHR> images;
    std::vector<GLuint> fbos;
    std::vector<GLuint> depth_rbs; // one depth renderbuffer per FBO
};

struct Renderer {
    Renderer();
    ~Renderer();
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&& other) noexcept;
    Renderer& operator=(Renderer&& other) noexcept;

    /// Must be called before xrCreateSession (EGL context needed for graphics binding).
    bool init();
    XrGraphicsBindingOpenGLESAndroidKHR graphics_binding() const;
    /// Requires init() to have succeeded and XrSession to exist.
    bool create_swapchains(XrInstance, XrSystemId, XrSession);
    /// EGL context must be current; must not be called when shouldRender is false. projLayer valid until next render_eyes call.
    bool render_eyes(XrInstance, XrSession, XrSpace refSpace, XrTime predictedDisplayTime,
                     std::span<const CubeInstance> cubes);

    /// Pointer valid only between render_eyes call and next render_eyes call; do not store across frames.
    const XrCompositionLayerProjection& proj_layer() const { return projLayer; }

private:
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig  config  = nullptr;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;

    std::array<EyeSwapchain, 2> eyes{};

    // Cached last-frame layer (valid until next render_eyes call)
    std::array<XrCompositionLayerProjectionView, 2> projViews{};
    XrCompositionLayerProjection projLayer{};

    std::unique_ptr<CubeMesh> cube_mesh_;
    bool     firstEyeRender_ = true;
    bool     layerLogged_    = false;
    double   lastLocateErr_  = 0.0;
};
