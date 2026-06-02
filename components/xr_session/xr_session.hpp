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
#include <functional>

struct XrSessionObj {
private:
    XrInstance   instance  = XR_NULL_HANDLE;
    XrSession    handle    = XR_NULL_HANDLE;
    XrSpace      worldSpace = XR_NULL_HANDLE;
    XrSessionState state   = XR_SESSION_STATE_UNKNOWN;
    bool         quit_           = false;
    bool         sessionRunning_ = false;
    bool         firstFrame_     = true;
    double       lastHeartbeat_  = 0.0;
    double       lastEndErr_     = 0.0;

public:

    ~XrSessionObj() {
        if (worldSpace != XR_NULL_HANDLE) { xrDestroySpace(worldSpace); }
        if (handle != XR_NULL_HANDLE) { xrDestroySession(handle); }
    }

    XrSessionObj() = default;
    XrSessionObj(const XrSessionObj&) = delete;
    XrSessionObj& operator=(const XrSessionObj&) = delete;

    XrSessionObj(XrSessionObj&& other) noexcept
        : instance(std::exchange(other.instance, XR_NULL_HANDLE))
        , handle(std::exchange(other.handle, XR_NULL_HANDLE))
        , worldSpace(std::exchange(other.worldSpace, XR_NULL_HANDLE))
        , state(std::exchange(other.state, XR_SESSION_STATE_UNKNOWN))
        , quit_(std::exchange(other.quit_, false))
        , sessionRunning_(std::exchange(other.sessionRunning_, false))
        , firstFrame_(std::exchange(other.firstFrame_, true))
        , lastHeartbeat_(std::exchange(other.lastHeartbeat_, 0.0))
        , lastEndErr_(std::exchange(other.lastEndErr_, 0.0)) {}

    XrSessionObj& operator=(XrSessionObj&& other) noexcept {
        if (this != &other) {
            if (worldSpace != XR_NULL_HANDLE) { xrDestroySpace(worldSpace); }
            if (handle != XR_NULL_HANDLE) { xrDestroySession(handle); }
            instance       = std::exchange(other.instance, XR_NULL_HANDLE);
            handle         = std::exchange(other.handle, XR_NULL_HANDLE);
            worldSpace     = std::exchange(other.worldSpace, XR_NULL_HANDLE);
            state          = std::exchange(other.state, XR_SESSION_STATE_UNKNOWN);
            quit_           = std::exchange(other.quit_, false);
            sessionRunning_ = std::exchange(other.sessionRunning_, false);
            firstFrame_     = std::exchange(other.firstFrame_, true);
            lastHeartbeat_  = std::exchange(other.lastHeartbeat_, 0.0);
            lastEndErr_     = std::exchange(other.lastEndErr_, 0.0);
        }
        return *this;
    }

    // The provided XrInstance must outlive this object.
    bool create(XrInstance, XrSystemId, const XrGraphicsBindingOpenGLESAndroidKHR&);
    void poll_events();
    // on_render is called between xrBeginFrame and xrEndFrame with predictedDisplayTime when shouldRender.
    // It returns the layers to submit (may be empty). If shouldRender is false, on_render is not called.
    void render_frame(std::function<std::vector<const XrCompositionLayerBaseHeader*>(XrTime)> on_render = {});

    [[nodiscard]] XrSession  get()             const { return handle; }
    [[nodiscard]] XrSpace    worldSpace_()     const { return worldSpace; }
    [[nodiscard]] bool       should_quit()     const { return quit_; }
    [[nodiscard]] bool       session_running() const { return sessionRunning_; }
    [[nodiscard]] bool       should_render()   const {
        return state == XR_SESSION_STATE_SYNCHRONIZED ||
               state == XR_SESSION_STATE_VISIBLE      ||
               state == XR_SESSION_STATE_FOCUSED;
    }
};
