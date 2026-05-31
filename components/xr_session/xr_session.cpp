#include "xr_session.hpp"
#include <android/log.h>

#define LOG(...)  __android_log_print(ANDROID_LOG_INFO,  "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

bool XrSessionObj::create(XrInstance instance, XrSystemId systemId,
                          const XrGraphicsBindingOpenGLESAndroidKHR& binding) {
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
