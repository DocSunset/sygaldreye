#include "xr_session.hpp"
#include <android/log.h>
#include <time.h>

#define LOG(...)  __android_log_print(ANDROID_LOG_INFO,  "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

static double now_sec() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

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

void XrSessionObj::poll_events() {
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
                if (XR_FAILED(r)) LOGE("xrBeginSession failed: %d", (int)r);
                else { LOG("xrBeginSession: success"); sessionRunning_ = true; }
            } else if (state == XR_SESSION_STATE_STOPPING) {
                sessionRunning_ = false;
                XrResult r = xrEndSession(handle);
                if (XR_FAILED(r)) LOGE("xrEndSession failed: %d", (int)r);
                else LOG("xrEndSession: success");
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
}

bool XrSessionObj::create(XrInstance inst, XrSystemId systemId,
                          const XrGraphicsBindingOpenGLESAndroidKHR& binding) {
    instance = inst;
    // Required before xrCreateSession per XR_KHR_opengl_es_enable spec
    PFN_xrGetOpenGLESGraphicsRequirementsKHR getReqs = nullptr;
    xrGetInstanceProcAddr(instance, "xrGetOpenGLESGraphicsRequirementsKHR",
                          (PFN_xrVoidFunction*)&getReqs);
    if (getReqs) {
        XrGraphicsRequirementsOpenGLESKHR req{XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR};
        getReqs(instance, systemId, &req);
    }

    XrSessionCreateInfo ci{XR_TYPE_SESSION_CREATE_INFO};
    ci.next     = &binding;
    ci.systemId = systemId;

    XrResult r = xrCreateSession(instance, &ci, &handle);
    if (XR_FAILED(r)) {
        LOGE("xrCreateSession failed: %d", (int)r);
        return false;
    }
    LOG("xrCreateSession: success, handle=%p", (void*)handle);

    // Enumerate supported reference space types
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
    if (XR_FAILED(rs)) {
        LOGE("xrCreateReferenceSpace failed: %d", (int)rs);
        return false;
    }
    LOG("xrCreateReferenceSpace: success, space=%p", (void*)worldSpace);
    return true;
}

void XrSessionObj::render_frame(
    std::function<std::vector<const XrCompositionLayerBaseHeader*>(XrTime)> on_render) {
    static double lastHeartbeat = 0;
    static bool firstFrame = true;

    if (firstFrame) { LOG("frame loop running"); firstFrame = false; }

    XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState{XR_TYPE_FRAME_STATE};
    XrResult r = xrWaitFrame(handle, &waitInfo, &frameState);
    if (XR_FAILED(r)) { LOGE("xrWaitFrame failed: %d", (int)r); return; }

    XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    r = xrBeginFrame(handle, &beginInfo);
    if (XR_FAILED(r)) { LOGE("xrBeginFrame failed: %d", (int)r); return; }

    std::vector<const XrCompositionLayerBaseHeader*> layers;
    if (frameState.shouldRender && on_render)
        layers = on_render(frameState.predictedDisplayTime);

    XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
    endInfo.displayTime          = frameState.predictedDisplayTime;
    endInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    if (!layers.empty()) {
        endInfo.layerCount = (uint32_t)layers.size();
        endInfo.layers     = layers.data();
    }
    r = xrEndFrame(handle, &endInfo);
    static double lastEndErr = 0;
    if (XR_FAILED(r)) {
        double t2 = now_sec();
        if (t2 - lastEndErr >= 2.0) { LOGE("xrEndFrame failed: %d", (int)r); lastEndErr = t2; }
        return;
    }

    double t = now_sec();
    if (t - lastHeartbeat >= 5.0) { LOG("frame loop heartbeat"); lastHeartbeat = t; }
}
