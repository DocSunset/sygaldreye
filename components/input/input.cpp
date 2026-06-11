// Copyright 2025 Travis West
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
        if (handSpaces_.at(static_cast<size_t>(i)) != XR_NULL_HANDLE)
            xrDestroySpace(handSpaces_.at(static_cast<size_t>(i)));
    }
    if (thumbstickAction_ != XR_NULL_HANDLE) xrDestroyAction(thumbstickAction_);
    if (gripAction_       != XR_NULL_HANDLE) xrDestroyAction(gripAction_);
    if (triggerAction_    != XR_NULL_HANDLE) xrDestroyAction(triggerAction_);
    if (poseAction_       != XR_NULL_HANDLE) xrDestroyAction(poseAction_);
    if (actionSet_        != XR_NULL_HANDLE) xrDestroyActionSet(actionSet_);
}

Input::Input(Input&& o) noexcept
    : actionSet_(std::exchange(o.actionSet_, XR_NULL_HANDLE))
    , poseAction_(std::exchange(o.poseAction_, XR_NULL_HANDLE))
    , triggerAction_(std::exchange(o.triggerAction_, XR_NULL_HANDLE))
    , gripAction_(std::exchange(o.gripAction_, XR_NULL_HANDLE))
    , thumbstickAction_(std::exchange(o.thumbstickAction_, XR_NULL_HANDLE))
    , handPaths_(o.handPaths_), poses_(o.poses_)
    , trigger_pressed_(o.trigger_pressed_), grip_pressed_(o.grip_pressed_)
    , thumbstick_(o.thumbstick_)
    , pose_logged_(std::exchange(o.pose_logged_, false))
{
    for (int i = 0; i < 2; ++i) {
        handSpaces_[static_cast<size_t>(i)]   = o.handSpaces_[static_cast<size_t>(i)];
        o.handSpaces_[static_cast<size_t>(i)] = XR_NULL_HANDLE;
    }
}

Input& Input::operator=(Input&& o) noexcept {
    if (this != &o) { this->~Input(); new(this) Input(std::move(o)); }
    return *this;
}

static bool make_bool_action(XrActionSet as, const char* name, const char* local,
                              XrPath* paths, uint32_t nPaths, XrAction* out) {
    XrActionCreateInfo ci{};
    ci.type = XR_TYPE_ACTION_CREATE_INFO;
    strncpy(ci.actionName, name, XR_MAX_ACTION_NAME_SIZE - 1);
    strncpy(ci.localizedActionName, local, XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    ci.actionType          = XR_ACTION_TYPE_BOOLEAN_INPUT;
    ci.countSubactionPaths = nPaths;
    ci.subactionPaths      = paths;
    return xr_ok(xrCreateAction(as, &ci, out), name);
}

static bool make_vec2_action(XrActionSet as, const char* name, const char* local,
                              XrPath* paths, uint32_t nPaths, XrAction* out) {
    XrActionCreateInfo ci{};
    ci.type = XR_TYPE_ACTION_CREATE_INFO;
    strncpy(ci.actionName, name, XR_MAX_ACTION_NAME_SIZE - 1);
    strncpy(ci.localizedActionName, local, XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    ci.actionType          = XR_ACTION_TYPE_VECTOR2F_INPUT;
    ci.countSubactionPaths = nPaths;
    ci.subactionPaths      = paths;
    return xr_ok(xrCreateAction(as, &ci, out), name);
}

bool Input::create(XrInstance instance, XrSession session) {
    XrActionSetCreateInfo asci{};
    asci.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    strncpy(asci.actionSetName, "gameplay", XR_MAX_ACTION_SET_NAME_SIZE - 1);
    strncpy(asci.localizedActionSetName, "Gameplay", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    asci.priority = 0;
    if (!xr_ok(xrCreateActionSet(instance, &asci, &actionSet_), "xrCreateActionSet"))
        return false;

    xrStringToPath(instance, "/user/hand/left",  &handPaths_[0]);
    xrStringToPath(instance, "/user/hand/right", &handPaths_[1]);

    // Pose action
    XrActionCreateInfo aci{};
    aci.type = XR_TYPE_ACTION_CREATE_INFO;
    strncpy(aci.actionName, "hand_pose", XR_MAX_ACTION_NAME_SIZE - 1);
    strncpy(aci.localizedActionName, "Hand Pose", XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
    aci.actionType          = XR_ACTION_TYPE_POSE_INPUT;
    aci.countSubactionPaths = 2;
    aci.subactionPaths      = handPaths_.data();
    if (!xr_ok(xrCreateAction(actionSet_, &aci, &poseAction_), "poseAction")) return false;

    if (!make_bool_action(actionSet_, "trigger", "Trigger",
                          handPaths_.data(), 2, &triggerAction_)) return false;
    if (!make_bool_action(actionSet_, "grip", "Grip",
                          handPaths_.data(), 2, &gripAction_)) return false;
    if (!make_vec2_action(actionSet_, "thumbstick", "Thumbstick",
                          handPaths_.data(), 2, &thumbstickAction_)) return false;
    // Face buttons are hand-specific paths; no subactions needed.
    if (!make_bool_action(actionSet_, "btn_x", "X", nullptr, 0, &xAction_)) return false;
    if (!make_bool_action(actionSet_, "btn_y", "Y", nullptr, 0, &yAction_)) return false;
    if (!make_bool_action(actionSet_, "btn_a", "A", nullptr, 0, &aAction_)) return false;
    if (!make_bool_action(actionSet_, "btn_b", "B", nullptr, 0, &bAction_)) return false;

    // Suggest bindings for Oculus Touch
    XrPath profile = XR_NULL_PATH;
    xrStringToPath(instance, "/interaction_profiles/oculus/touch_controller", &profile);

    XrPath paths[12];
    xrStringToPath(instance, "/user/hand/left/input/aim/pose",          &paths[0]);
    xrStringToPath(instance, "/user/hand/right/input/aim/pose",         &paths[1]);
    xrStringToPath(instance, "/user/hand/left/input/trigger/value",     &paths[2]);
    xrStringToPath(instance, "/user/hand/right/input/trigger/value",    &paths[3]);
    xrStringToPath(instance, "/user/hand/left/input/squeeze/value",     &paths[4]);
    xrStringToPath(instance, "/user/hand/right/input/squeeze/value",    &paths[5]);
    xrStringToPath(instance, "/user/hand/left/input/thumbstick",        &paths[6]);
    xrStringToPath(instance, "/user/hand/right/input/thumbstick",       &paths[7]);
    xrStringToPath(instance, "/user/hand/left/input/x/click",           &paths[8]);
    xrStringToPath(instance, "/user/hand/left/input/y/click",           &paths[9]);
    xrStringToPath(instance, "/user/hand/right/input/a/click",          &paths[10]);
    xrStringToPath(instance, "/user/hand/right/input/b/click",          &paths[11]);

    XrActionSuggestedBinding bindings[12] = {
        {poseAction_,       paths[0]}, {poseAction_,        paths[1]},
        {triggerAction_,    paths[2]}, {triggerAction_,     paths[3]},
        {gripAction_,       paths[4]}, {gripAction_,        paths[5]},
        {thumbstickAction_, paths[6]}, {thumbstickAction_,  paths[7]},
        {xAction_,          paths[8]}, {yAction_,           paths[9]},
        {aAction_,          paths[10]}, {bAction_,          paths[11]},
    };
    XrInteractionProfileSuggestedBinding suggested{};
    suggested.type                   = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
    suggested.interactionProfile     = profile;
    suggested.suggestedBindings      = bindings;
    suggested.countSuggestedBindings = 12;
    if (!xr_ok(xrSuggestInteractionProfileBindings(instance, &suggested),
               "xrSuggestInteractionProfileBindings")) return false;

    XrSessionActionSetsAttachInfo attachInfo{};
    attachInfo.type            = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    attachInfo.actionSets      = &actionSet_;
    attachInfo.countActionSets = 1;
    if (!xr_ok(xrAttachSessionActionSets(session, &attachInfo), "xrAttachSessionActionSets"))
        return false;

    for (int i = 0; i < 2; ++i) {
        XrActionSpaceCreateInfo sci{};
        sci.type              = XR_TYPE_ACTION_SPACE_CREATE_INFO;
        sci.action            = poseAction_;
        sci.subactionPath     = handPaths_[static_cast<size_t>(i)];
        sci.poseInActionSpace = {{0,0,0,1},{0,0,0}};
        if (!xr_ok(xrCreateActionSpace(session, &sci,
                   &handSpaces_[static_cast<size_t>(i)]), "xrCreateActionSpace"))
            return false;
    }

    LOGI("created");
    return true;
}

bool Input::sync(XrSession session, XrSpace worldSpace, XrTime time, bool focused) {
    if (!focused) {
        for (auto& p : poses_) p = std::nullopt;
        return true;
    }
    XrActiveActionSet active{actionSet_, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo{};
    syncInfo.type                  = XR_TYPE_ACTIONS_SYNC_INFO;
    syncInfo.activeActionSets      = &active;
    syncInfo.countActiveActionSets = 1;
    if (!xr_ok(xrSyncActions(session, &syncInfo), "xrSyncActions")) return false;

    for (int i = 0; i < 2; ++i) {
        size_t si = static_cast<size_t>(i);
        XrActionStateGetInfo gi{};
        gi.type          = XR_TYPE_ACTION_STATE_GET_INFO;
        gi.subactionPath = handPaths_[si];

        // Trigger
        XrActionStateBoolean tb{};
        tb.type = XR_TYPE_ACTION_STATE_BOOLEAN;
        gi.action = triggerAction_;
        xrGetActionStateBoolean(session, &gi, &tb);
        trigger_pressed_[si] = (tb.isActive && tb.currentState);

        // Grip
        XrActionStateBoolean gb{};
        gb.type = XR_TYPE_ACTION_STATE_BOOLEAN;
        gi.action = gripAction_;
        xrGetActionStateBoolean(session, &gi, &gb);
        grip_pressed_[si] = (gb.isActive && gb.currentState);

        // Face buttons (hand-specific; query without subaction)
        XrActionStateGetInfo bi{};
        bi.type = XR_TYPE_ACTION_STATE_GET_INFO;
        XrActionStateBoolean bb{};
        bb.type = XR_TYPE_ACTION_STATE_BOOLEAN;
        bi.action = (i == 0) ? xAction_ : aAction_;
        xrGetActionStateBoolean(session, &bi, &bb);
        btn1_pressed_[si] = (bb.isActive && bb.currentState);
        bb = {}; bb.type = XR_TYPE_ACTION_STATE_BOOLEAN;
        bi.action = (i == 0) ? yAction_ : bAction_;
        xrGetActionStateBoolean(session, &bi, &bb);
        btn2_pressed_[si] = (bb.isActive && bb.currentState);

        // Thumbstick
        XrActionStateVector2f vb{};
        vb.type = XR_TYPE_ACTION_STATE_VECTOR2F;
        gi.action = thumbstickAction_;
        xrGetActionStateVector2f(session, &gi, &vb);
        thumbstick_[si] = (vb.isActive)
            ? Eigen::Vector2f{vb.currentState.x, vb.currentState.y}
            : Eigen::Vector2f{0.f, 0.f};

        // Pose
        XrSpaceLocation loc{};
        loc.type = XR_TYPE_SPACE_LOCATION;
        xrLocateSpace(handSpaces_[si], worldSpace, time, &loc);
        constexpr XrSpaceLocationFlags valid =
            XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
        if ((loc.locationFlags & valid) == valid) {
            poses_[si] = HandPose{loc.pose};
            if (!pose_logged_) {
                LOGI("hand %d pos %.3f %.3f %.3f", i,
                     loc.pose.position.x, loc.pose.position.y, loc.pose.position.z);
                pose_logged_ = true;
            }
        } else {
            poses_[si] = std::nullopt;
        }
    }
    return true;
}

std::optional<HandPose> Input::hand_pose(Hand hand) const {
    return poses_[static_cast<size_t>(hand)];
}
bool Input::trigger_pressed(Hand hand) const {
    return trigger_pressed_[static_cast<size_t>(hand)];
}
bool Input::grip_pressed(Hand hand) const {
    return grip_pressed_[static_cast<size_t>(hand)];
}
Eigen::Vector2f Input::thumbstick(Hand hand) const {
    return thumbstick_[static_cast<size_t>(hand)];
}

bool Input::button1_pressed(Hand hand) const { return btn1_pressed_[static_cast<size_t>(hand)]; }
bool Input::button2_pressed(Hand hand) const { return btn2_pressed_[static_cast<size_t>(hand)]; }
