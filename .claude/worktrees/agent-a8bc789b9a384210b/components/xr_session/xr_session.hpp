#pragma once
#include <openxr/openxr.h>
#include <utility>

struct XrSessionObj {
private:
    XrInstance     instance   = XR_NULL_HANDLE;
    XrSession      handle     = XR_NULL_HANDLE;
    XrSpace        worldSpace = XR_NULL_HANDLE;
    XrSessionState state      = XR_SESSION_STATE_UNKNOWN;
    bool           quit_           = false;
    bool           sessionRunning_ = false;

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
        , sessionRunning_(std::exchange(other.sessionRunning_, false)) {}

    XrSessionObj& operator=(XrSessionObj&& other) noexcept {
        if (this != &other) {
            if (worldSpace != XR_NULL_HANDLE) { xrDestroySpace(worldSpace); }
            if (handle != XR_NULL_HANDLE) { xrDestroySession(handle); }
            instance        = std::exchange(other.instance, XR_NULL_HANDLE);
            handle          = std::exchange(other.handle, XR_NULL_HANDLE);
            worldSpace      = std::exchange(other.worldSpace, XR_NULL_HANDLE);
            state           = std::exchange(other.state, XR_SESSION_STATE_UNKNOWN);
            quit_           = std::exchange(other.quit_, false);
            sessionRunning_ = std::exchange(other.sessionRunning_, false);
        }
        return *this;
    }

    bool create(XrInstance, XrSystemId, const void* graphics_binding);
    bool poll_events();

    [[nodiscard]] XrSession get()             const { return handle; }
    [[nodiscard]] XrSpace   worldSpace_()     const { return worldSpace; }
    [[nodiscard]] bool      should_quit()     const { return quit_; }
    [[nodiscard]] bool      session_running() const { return sessionRunning_; }
    [[nodiscard]] bool      should_render()   const {
        return state == XR_SESSION_STATE_SYNCHRONIZED ||
               state == XR_SESSION_STATE_VISIBLE      ||
               state == XR_SESSION_STATE_FOCUSED;
    }
};
