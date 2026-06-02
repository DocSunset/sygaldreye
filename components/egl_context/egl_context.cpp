#include "egl_context.hpp"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <utility>

#define LOG(...)  __android_log_print(ANDROID_LOG_INFO,  "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

EglContext::~EglContext() {
    if (surface != EGL_NO_SURFACE) { eglDestroySurface(display, surface); }
    if (context != EGL_NO_CONTEXT) { eglDestroyContext(display, context); }
    if (display != EGL_NO_DISPLAY) { eglTerminate(display); }
}

EglContext::EglContext(EglContext&& other) noexcept
    : display(std::exchange(other.display, EGL_NO_DISPLAY))
    , config(std::exchange(other.config, nullptr))
    , context(std::exchange(other.context, EGL_NO_CONTEXT))
    , surface(std::exchange(other.surface, EGL_NO_SURFACE))
{}

EglContext& EglContext::operator=(EglContext&& other) noexcept {
    if (this != &other) {
        this->~EglContext();
        display = std::exchange(other.display, EGL_NO_DISPLAY);
        config  = std::exchange(other.config, nullptr);
        context = std::exchange(other.context, EGL_NO_CONTEXT);
        surface = std::exchange(other.surface, EGL_NO_SURFACE);
    }
    return *this;
}

std::optional<RendererBinding> EglContext::init() {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) { LOGE("eglGetDisplay failed"); return std::nullopt; }

    if (!eglInitialize(display, nullptr, nullptr)) {
        LOGE("eglInitialize failed: 0x%x", eglGetError()); return std::nullopt;
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
        LOGE("eglChooseConfig failed: 0x%x", eglGetError()); return std::nullopt;
    }

    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed: 0x%x", eglGetError()); return std::nullopt;
    }

    const EGLint pbAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    surface = eglCreatePbufferSurface(display, config, pbAttribs);
    if (surface == EGL_NO_SURFACE) {
        LOGE("eglCreatePbufferSurface failed: 0x%x", eglGetError()); return std::nullopt;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOGE("eglMakeCurrent failed: 0x%x", eglGetError()); return std::nullopt;
    }

    LOG("eglMakeCurrent success");
    LOG("GL_VERSION:  %s", glGetString(GL_VERSION));
    LOG("GL_RENDERER: %s", glGetString(GL_RENDERER));
    LOG("GL_VENDOR:   %s", glGetString(GL_VENDOR));
    return RendererBinding{
        .xr_binding = {
            .type    = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
            .next    = nullptr,
            .display = display,
            .config  = config,
            .context = context,
        }
    };
}
