#pragma once
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <vector>
#include <array>

struct EyeSwapchain {
    XrSwapchain handle = XR_NULL_HANDLE;
    uint32_t    width  = 0;
    uint32_t    height = 0;
    std::vector<XrSwapchainImageOpenGLESKHR> images;
    std::vector<GLuint> fbos;

    // returns FBO for the given acquired image index
    GLuint fbo(uint32_t index) const { return fbos[index]; }
};

struct Renderer {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig  config  = nullptr;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;

    std::array<EyeSwapchain, 2> eyes{};

    bool init();
    XrGraphicsBindingOpenGLESAndroidKHR graphics_binding() const;
    bool create_swapchains(XrInstance, XrSystemId, XrSession);
    void render_eyes(XrTime predictedDisplayTime);
};
