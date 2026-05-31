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
    return true;
}
