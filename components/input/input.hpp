#pragma once
#include <openxr/openxr.h>
#include <Eigen/Core>
#include <array>
#include <cstdint>
#include <optional>

enum class Hand : uint8_t { LEFT = 0, RIGHT = 1 };

struct HandPose {
    XrPosef pose;
};

struct Input {
    Input() = default;
    ~Input();

    Input(const Input&)            = delete;
    Input& operator=(const Input&) = delete;

    Input(Input&& other) noexcept;
    Input& operator=(Input&& other) noexcept;

    /// Must succeed before sync() is safe to call.
    bool create(XrInstance instance, XrSession session);
    /// Must be called between xrBeginFrame and xrEndFrame; session, worldSpace, and time must be from same frame; not thread-safe.
    /// focused must be true only when the session is in SYNCHRONIZED, VISIBLE, or FOCUSED state.
    bool sync(XrSession session, XrSpace worldSpace, XrTime time, bool focused);
    [[nodiscard]] std::optional<HandPose> hand_pose(Hand hand) const;
    [[nodiscard]] bool trigger_pressed(Hand hand) const;
    [[nodiscard]] bool grip_pressed(Hand hand) const;
    [[nodiscard]] Eigen::Vector2f thumbstick(Hand hand) const;

private:
    XrActionSet actionSet_       = XR_NULL_HANDLE;
    XrAction    poseAction_      = XR_NULL_HANDLE;
    XrAction    triggerAction_   = XR_NULL_HANDLE;
    XrAction    gripAction_      = XR_NULL_HANDLE;
    XrAction    thumbstickAction_= XR_NULL_HANDLE;
    std::array<XrPath, 2>               handPaths_{XR_NULL_PATH, XR_NULL_PATH};
    std::array<XrSpace, 2>              handSpaces_{XR_NULL_HANDLE, XR_NULL_HANDLE};
    std::array<std::optional<HandPose>, 2> poses_{};
    std::array<bool, 2>                 trigger_pressed_{};
    std::array<bool, 2>                 grip_pressed_{};
    std::array<Eigen::Vector2f, 2>      thumbstick_{};
    bool        pose_logged_ = false;
};
