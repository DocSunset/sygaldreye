#include "renderer.hpp"
#include <GLES3/gl3.h>
#include <android/log.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

bool Renderer::init() {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) { LOGE("eglGetDisplay failed"); return false; }

    if (!eglInitialize(display, nullptr, nullptr)) {
        LOGE("eglInitialize failed: 0x%x", eglGetError()); return false;
    }

    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
        EGL_RED_SIZE,   8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0,
        EGL_NONE
    };
    EGLint numConfigs = 0;
    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs) || numConfigs < 1) {
        LOGE("eglChooseConfig failed: 0x%x", eglGetError()); return false;
    }

    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed: 0x%x", eglGetError()); return false;
    }

    const EGLint pbAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    surface = eglCreatePbufferSurface(display, config, pbAttribs);
    if (surface == EGL_NO_SURFACE) {
        LOGE("eglCreatePbufferSurface failed: 0x%x", eglGetError()); return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOGE("eglMakeCurrent failed: 0x%x", eglGetError()); return false;
    }

    LOG("eglMakeCurrent success");
    LOG("GL_VERSION:  %s", glGetString(GL_VERSION));
    LOG("GL_RENDERER: %s", glGetString(GL_RENDERER));
    LOG("GL_VENDOR:   %s", glGetString(GL_VENDOR));
    return true;
}
