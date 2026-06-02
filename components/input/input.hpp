#pragma once
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <array>

struct HandPose {
    XrPosef pose;
    bool    valid = false;
};

struct Input {
    Input() = default;
    ~Input();

    Input(const Input&)            = delete;
    Input& operator=(const Input&) = delete;

    Input(Input&& other) noexcept;
    Input& operator=(Input&& other) noexcept;

    bool create(XrInstance instance, XrSession session);
    void sync(XrSession session, XrSpace worldSpace, XrTime time);
    [[nodiscard]] HandPose hand_pose(int hand) const;  // hand: 0=left, 1=right

private:
    XrActionSet actionSet_   = XR_NULL_HANDLE;
    XrAction    poseAction_  = XR_NULL_HANDLE;
    std::array<XrSpace, 2>   handSpaces_{XR_NULL_HANDLE, XR_NULL_HANDLE};
    std::array<HandPose, 2>  poses_{};
    bool        pose_logged_ = false;
};
