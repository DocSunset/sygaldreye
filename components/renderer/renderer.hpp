#pragma once
#include <EGL/egl.h>

struct Renderer {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig  config  = nullptr;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;

    bool init();
};
