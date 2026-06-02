#include "input.hpp"
#include <android/log.h>
#include <utility>

#define TAG "input"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static bool xr_ok(XrResult res, const char* what) {
    if (XR_SUCCEEDED(res)) { return true; }
    LOGE("%s failed: %d", what, static_cast<int>(res));
    return false;
}

Input::~Input() {
    for (int i = 0; i < 2; ++i) {
        if (handSpaces_.at(static_cast<size_t>(i)) != XR_NULL_HANDLE) {
            xrDestroySpace(handSpaces_.at(static_cast<size_t>(i)));
        }
    }
    if (poseAction_ != XR_NULL_HANDLE) {
        xrDestroyAction(poseAction_);
    }
    if (actionSet_ != XR_NULL_HANDLE) {
        xrDestroyActionSet(actionSet_);
    }
}

Input::Input(Input&& other) noexcept
    : actionSet_(std::exchange(other.actionSet_, XR_NULL_HANDLE))
    , poseAction_(std::exchange(other.poseAction_, XR_NULL_HANDLE))
    , poses_(other.poses_)
    , pose_logged_(std::exchange(other.pose_logged_, false))
{
    for (int i = 0; i < 2; ++i) {
        handSpaces_.at(static_cast<size_t>(i))       = other.handSpaces_.at(static_cast<size_t>(i));
        other.handSpaces_.at(static_cast<size_t>(i)) = XR_NULL_HANDLE;
    }
}

Input& Input::operator=(Input&& other) noexcept {
    if (this != &other) {
        this->~Input();
        actionSet_  = std::exchange(other.actionSet_,  XR_NULL_HANDLE);
        poseAction_ = std::exchange(other.poseAction_, XR_NULL_HANDLE);
        poses_       = other.poses_;
        pose_logged_ = std::exchange(other.pose_logged_, false);
        for (int i = 0; i < 2; ++i) {
            handSpaces_.at(static_cast<size_t>(i))       = other.handSpaces_.at(static_cast<size_t>(i));
            other.handSpaces_.at(static_cast<size_t>(i)) = XR_NULL_HANDLE;
        }
    }
    return *this;
}

bool Input::create(XrInstance instance, XrSession session) {
    // Action set
    XrActionSetCreateInfo asci{XR_TYPE_ACTION_SET_CREATE_INFO};
    strncpy(asci.actionSetName, "gameplay", XR_MAX_ACTION_SET_NAME_SIZE - 1);
    asci.actionSetName[XR_MAX_ACTION_SET_NAME_SIZE - 1] = '\0';
    strncpy(asci.localizedActionSetName, "Gameplay", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    asci.localizedActionSetName[XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1] = '\0';
    asci.priority = 0;
    if (!xr_ok(xrCreateActionSet(instance, &asci, &actionSet_), "xrCreateActionSet")) {
        return false;
    }

    // Grip pose action with left/right subaction paths
    XrPath handPaths[2];
    xrStringToPath(instance, "/user/hand/left",  &handPaths[0]);
    xrStringToPath(instance, "/user/hand/right", &handPaths[1]);

    XrActionCreateInfo aci{XR_TYPE_ACTION_CREATE_INFO};
    strncpy(aci.actionName, "hand_pose", XR_MAX_ACTION_NAME_SIZE - 1);
    aci.actionName[XR_MAX_ACTION_NAME_SIZE - 1] = '\0';
    strncpy(aci.localizedActionName, "Hand Pose", XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
    aci.localizedActionName[XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1] = '\0';
    aci.actionType          = XR_ACTION_TYPE_POSE_INPUT;
    aci.countSubactionPaths = 2;
    aci.subactionPaths      = handPaths;
    if (!xr_ok(xrCreateAction(actionSet_, &aci, &poseAction_), "xrCreateAction")) {
        return false;
    }

    // Suggest bindings for Oculus Touch
    XrPath profilePath = XR_NULL_PATH;
    xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller", &profilePath);

    XrPath leftPath  = XR_NULL_PATH;
    XrPath rightPath = XR_NULL_PATH;
    xrStringToPath(instance, "/user/hand/left/input/grip/pose",  &leftPath);
    xrStringToPath(instance, "/user/hand/right/input/grip/pose", &rightPath);

    XrActionSuggestedBinding bindings[2] = {
        {poseAction_, leftPath},
        {poseAction_, rightPath},
    };
    XrInteractionProfileSuggestedBinding suggested{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggested.interactionProfile     = profilePath;
    suggested.suggestedBindings      = bindings;
    suggested.countSuggestedBindings = 2;
    if (!xr_ok(xrSuggestInteractionProfileBindings(instance, &suggested),
               "xrSuggestInteractionProfileBindings")) {
        return false;
    }

    // Attach action set
    XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attachInfo.actionSets      = &actionSet_;
    attachInfo.countActionSets = 1;
    if (!xr_ok(xrAttachSessionActionSets(session, &attachInfo), "xrAttachSessionActionSets")) {
        return false;
    }

    // Create hand spaces
    for (int i = 0; i < 2; ++i) {
        XrActionSpaceCreateInfo spaceCi{XR_TYPE_ACTION_SPACE_CREATE_INFO};
        spaceCi.action            = poseAction_;
        spaceCi.subactionPath     = handPaths[i];
        spaceCi.poseInActionSpace = {{0,0,0,1},{0,0,0}};
        if (!xr_ok(xrCreateActionSpace(session, &spaceCi, &handSpaces_.at(static_cast<size_t>(i))), "xrCreateActionSpace")) {
            return false;
        }
    }

    LOGI("created");
    return true;
}

bool Input::sync(XrSession session, XrSpace worldSpace, XrTime time, bool focused) {
    if (!focused) {
        for (size_t i = 0; i < poses_.size(); ++i) { poses_.at(i) = std::nullopt; }
        return true;
    }
    XrActiveActionSet active{actionSet_, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
    syncInfo.activeActionSets      = &active;
    syncInfo.countActiveActionSets = 1;
    if (!xr_ok(xrSyncActions(session, &syncInfo), "xrSyncActions")) {
        return false;
    }

    for (int i = 0; i < 2; ++i) {
        XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION};
        xrLocateSpace(handSpaces_.at(static_cast<size_t>(i)), worldSpace, time, &loc);
        constexpr XrSpaceLocationFlags valid =
            XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
        if ((loc.locationFlags & valid) == valid) {
            poses_[i] = HandPose{loc.pose};
            if (!pose_logged_) {
                LOGI("hand %d pos %.3f %.3f %.3f", i,
                     loc.pose.position.x, loc.pose.position.y, loc.pose.position.z);
                pose_logged_ = true;
            }
        } else {
            poses_[i] = std::nullopt;
        }
    }
    return true;
}

std::optional<HandPose> Input::hand_pose(Hand hand) const {
    return poses_[static_cast<size_t>(hand)];
}
