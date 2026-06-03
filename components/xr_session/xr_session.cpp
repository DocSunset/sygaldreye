#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <openxr/openxr_platform.h>
#include "xr_session.hpp"
#include <android/log.h>

#define LOG(...)  __android_log_print(ANDROID_LOG_INFO,  "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

static const char* session_state_str(XrSessionState s) {
    switch (s) {
#define C(x) case x: return #x
        C(XR_SESSION_STATE_UNKNOWN); C(XR_SESSION_STATE_IDLE);
        C(XR_SESSION_STATE_READY);   C(XR_SESSION_STATE_SYNCHRONIZED);
        C(XR_SESSION_STATE_VISIBLE); C(XR_SESSION_STATE_FOCUSED);
        C(XR_SESSION_STATE_STOPPING);C(XR_SESSION_STATE_LOSS_PENDING);
        C(XR_SESSION_STATE_EXITING);
#undef C
        default: return "UNKNOWN";
    }
}

bool XrSessionObj::poll_events() {
    XrEventDataBuffer ev{XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(instance, &ev) == XR_SUCCESS) {
        switch (ev.type) {
        case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
            auto& sc = *reinterpret_cast<XrEventDataSessionStateChanged*>(&ev);
            LOG("session state: %s -> %s", session_state_str(state), session_state_str(sc.state));
            state = sc.state;
            if (state == XR_SESSION_STATE_READY) {
                XrSessionBeginInfo bi{XR_TYPE_SESSION_BEGIN_INFO};
                bi.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                XrResult r = xrBeginSession(handle, &bi);
                if (XR_FAILED(r)) { LOGE("xrBeginSession failed: %d", (int)r); return false; }
                LOG("xrBeginSession: success");
                sessionRunning_ = true;
            } else if (state == XR_SESSION_STATE_STOPPING) {
                sessionRunning_ = false;
                XrResult r = xrEndSession(handle);
                if (XR_FAILED(r)) { LOGE("xrEndSession failed: %d", (int)r); return false; }
                LOG("xrEndSession: success");
            } else if (state == XR_SESSION_STATE_EXITING ||
                       state == XR_SESSION_STATE_LOSS_PENDING) {
                quit_ = true;
            }
            break;
        }
        case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            LOG("instance loss pending — quitting");
            quit_ = true;
            break;
        default: break;
        }
        ev = {XR_TYPE_EVENT_DATA_BUFFER};
    }
    return true;
}

bool XrSessionObj::create(XrInstance inst, XrSystemId systemId,
                          const void* graphics_binding) {
    instance = inst;
    PFN_xrGetOpenGLESGraphicsRequirementsKHR getReqs = nullptr;
    xrGetInstanceProcAddr(instance, "xrGetOpenGLESGraphicsRequirementsKHR",
                          reinterpret_cast<PFN_xrVoidFunction*>(&getReqs));
    if (getReqs) {
        XrGraphicsRequirementsOpenGLESKHR req{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
        getReqs(instance, systemId, &req);
    }

    XrSessionCreateInfo ci{XR_TYPE_SESSION_CREATE_INFO};
    ci.next     = graphics_binding;
    ci.systemId = systemId;

    XrResult r = xrCreateSession(instance, &ci, &handle);
    if (XR_FAILED(r)) { LOGE("xrCreateSession failed: %d", (int)r); return false; }
    LOG("xrCreateSession: success, handle=%p", (void*)handle);

    uint32_t count = 0;
    xrEnumerateReferenceSpaces(handle, 0, &count, nullptr);
    XrReferenceSpaceType types[16] = {};
    if (count > 16) count = 16;
    xrEnumerateReferenceSpaces(handle, count, &count, types);

    XrReferenceSpaceType chosen = XR_REFERENCE_SPACE_TYPE_LOCAL;
    for (uint32_t i = 0; i < count; i++) {
        if (types[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
            chosen = XR_REFERENCE_SPACE_TYPE_STAGE;
            break;
        }
    }
    LOG("reference space chosen: %s", chosen == XR_REFERENCE_SPACE_TYPE_STAGE ? "STAGE" : "LOCAL");

    XrReferenceSpaceCreateInfo rsci{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    rsci.referenceSpaceType = chosen;
    rsci.poseInReferenceSpace = {{0, 0, 0, 1}, {0, 0, 0}};

    XrResult rs = xrCreateReferenceSpace(handle, &rsci, &worldSpace);
    if (XR_FAILED(rs)) { LOGE("xrCreateReferenceSpace failed: %d", (int)rs); return false; }
    LOG("xrCreateReferenceSpace: success, space=%p", (void*)worldSpace);
    return true;
}
