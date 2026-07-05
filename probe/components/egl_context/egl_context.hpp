#pragma once
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <optional>

struct RendererBinding {
    XrGraphicsBindingOpenGLESAndroidKHR xr_binding;
};

struct EglContext {
    EglContext() = default;
    ~EglContext();
    EglContext(const EglContext&) = delete;
    EglContext& operator=(const EglContext&) = delete;
    EglContext(EglContext&&) noexcept;
    EglContext& operator=(EglContext&&) noexcept;

    [[nodiscard]] std::optional<RendererBinding> init();

private:
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig  config  = nullptr;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
};
