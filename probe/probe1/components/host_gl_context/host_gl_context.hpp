#pragma once
#include <EGL/egl.h>
#include <optional>

struct HostGlContext {
    static std::optional<HostGlContext> create();
    ~HostGlContext();
    HostGlContext(HostGlContext&&) noexcept;
    HostGlContext& operator=(HostGlContext&&) noexcept;
    HostGlContext(HostGlContext const&) = delete;
    HostGlContext& operator=(HostGlContext const&) = delete;
private:
    HostGlContext() = default;
    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLContext context_ = EGL_NO_CONTEXT;
    EGLSurface surface_ = EGL_NO_SURFACE;
};
