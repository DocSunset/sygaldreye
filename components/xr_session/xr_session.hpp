#pragma once
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <span>
#include <functional>

struct XrSessionObj {
    XrInstance   instance  = XR_NULL_HANDLE;
    XrSession    handle    = XR_NULL_HANDLE;
    XrSpace      worldSpace = XR_NULL_HANDLE;
    XrSessionState state   = XR_SESSION_STATE_UNKNOWN;
    bool         quit_     = false;
    bool         sessionRunning_ = false;

    bool create(XrInstance, XrSystemId, const XrGraphicsBindingOpenGLESAndroidKHR&);
    void poll_events();
    // Seam: caller may pass projection layers; defaulting to none runs the cycle with zero layers.
    // on_render is called between xrBeginFrame and xrEndFrame with predictedDisplayTime when shouldRender is true.
    void render_frame(std::span<const XrCompositionLayerBaseHeader* const> layers = {},
                      std::function<void(XrTime)> on_render = {});

    XrSession  get()          const { return handle; }
    XrSpace    worldSpace_()  const { return worldSpace; }
    bool       should_quit()  const { return quit_; }
    bool       session_running() const { return sessionRunning_; }
    bool       should_render() const {
        return state == XR_SESSION_STATE_SYNCHRONIZED ||
               state == XR_SESSION_STATE_VISIBLE      ||
               state == XR_SESSION_STATE_FOCUSED;
    }
};
