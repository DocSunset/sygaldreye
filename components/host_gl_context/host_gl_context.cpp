// Copyright 2025 Travis West
#include "host_gl_context.hpp"
#include <utility>

HostGlContext::~HostGlContext() {
    if (surface_ != EGL_NO_SURFACE) { eglDestroySurface(display_, surface_); }
    if (context_ != EGL_NO_CONTEXT) { eglDestroyContext(display_, context_); }
    if (display_ != EGL_NO_DISPLAY) { eglTerminate(display_); }
}

HostGlContext::HostGlContext(HostGlContext&& other) noexcept
    : display_(std::exchange(other.display_, EGL_NO_DISPLAY))
    , context_(std::exchange(other.context_, EGL_NO_CONTEXT))
    , surface_(std::exchange(other.surface_, EGL_NO_SURFACE))
{}

HostGlContext& HostGlContext::operator=(HostGlContext&& other) noexcept {
    if (this != &other) {
        this->~HostGlContext();
        display_ = std::exchange(other.display_, EGL_NO_DISPLAY);
        context_ = std::exchange(other.context_, EGL_NO_CONTEXT);
        surface_ = std::exchange(other.surface_, EGL_NO_SURFACE);
    }
    return *this;
}

std::optional<HostGlContext> HostGlContext::create() {
    HostGlContext ctx;

    ctx.display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (ctx.display_ == EGL_NO_DISPLAY) { return std::nullopt; }

    if (!eglInitialize(ctx.display_, nullptr, nullptr)) { return std::nullopt; }

    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
        EGL_RED_SIZE,   8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE
    };
    EGLConfig config = nullptr;
    EGLint numConfigs = 0;
    if (!eglChooseConfig(ctx.display_, configAttribs, &config, 1, &numConfigs) || numConfigs < 1) {
        return std::nullopt;
    }

    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    ctx.context_ = eglCreateContext(ctx.display_, config, EGL_NO_CONTEXT, ctxAttribs);
    if (ctx.context_ == EGL_NO_CONTEXT) { return std::nullopt; }

    const EGLint pbAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    ctx.surface_ = eglCreatePbufferSurface(ctx.display_, config, pbAttribs);
    if (ctx.surface_ == EGL_NO_SURFACE) { return std::nullopt; }

    if (!eglMakeCurrent(ctx.display_, ctx.surface_, ctx.surface_, ctx.context_)) {
        return std::nullopt;
    }

    return ctx;
}
