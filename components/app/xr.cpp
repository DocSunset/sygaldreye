#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <android/log.h>
#include <android_native_app_glue.h>

#include <cstring>
#include <vector>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

#define XR_CHECK(call) \
    do { \
        XrResult _r = (call); \
        if (XR_FAILED(_r)) { LOGE(#call " failed: %d", (int)_r); return XR_NULL_HANDLE; } \
    } while (0)

#define XR_CHECK_SYS(call) \
    do { \
        XrResult _r = (call); \
        if (XR_FAILED(_r)) { LOGE(#call " failed: %d", (int)_r); return XR_NULL_SYSTEM_ID; } \
    } while (0)

XrSystemId xr_get_system(XrInstance instance) {
    XrSystemGetInfo sgi{XR_TYPE_SYSTEM_GET_INFO};
    sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XR_CHECK_SYS(xrGetSystem(instance, &sgi, &systemId));
    LOG("xrGetSystem: success, systemId=%llu", (unsigned long long)systemId);

    XrSystemGraphicsProperties gfx{};
    XrSystemTrackingProperties trk{};
    XrSystemProperties props{XR_TYPE_SYSTEM_PROPERTIES};
    props.graphicsProperties = gfx;
    props.trackingProperties = trk;
    XrResult r = xrGetSystemProperties(instance, systemId, &props);
    if (XR_FAILED(r)) {
        LOGE("xrGetSystemProperties failed: %d", (int)r);
    } else {
        LOG("systemName=%s vendorId=%u", props.systemName, props.vendorId);
        LOG("maxSwapchainImageWidth=%u maxSwapchainImageHeight=%u maxLayerCount=%u",
            props.graphicsProperties.maxSwapchainImageWidth,
            props.graphicsProperties.maxSwapchainImageHeight,
            props.graphicsProperties.maxLayerCount);
        LOG("orientationTracking=%d positionTracking=%d",
            props.trackingProperties.orientationTracking,
            props.trackingProperties.positionTracking);
    }
    return systemId;
}

XrInstance xr_create_instance(struct android_app* app) {
    // Init loader
    PFN_xrInitializeLoaderKHR initLoader = nullptr;
    XR_CHECK(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                                   reinterpret_cast<PFN_xrVoidFunction*>(&initLoader)));
    XrLoaderInitInfoAndroidKHR loaderInfo{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
    loaderInfo.applicationVM      = app->activity->vm;
    loaderInfo.applicationContext = app->activity->clazz;
    XR_CHECK(initLoader((const XrLoaderInitInfoBaseHeaderKHR*)&loaderInfo));

    // Enumerate extensions
    const char* required[] = {
        XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME,
        XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME,
    };
    uint32_t extCount = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
    std::vector<XrExtensionProperties> exts(extCount, {XR_TYPE_EXTENSION_PROPERTIES});
    xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, exts.data());
    for (const char* req : required) {
        bool found = false;
        for (auto& e : exts) if (strcmp(e.extensionName, req) == 0) { found = true; break; }
        if (!found) LOGE("missing extension: %s", req);
    }

    // Create instance
    XrInstanceCreateInfoAndroidKHR androidInfo{XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
    androidInfo.applicationVM       = app->activity->vm;
    androidInfo.applicationActivity = app->activity->clazz;

    XrInstanceCreateInfo ci{XR_TYPE_INSTANCE_CREATE_INFO};
    ci.next                       = &androidInfo;
    ci.enabledExtensionCount      = 2;
    ci.enabledExtensionNames      = required;
    ci.applicationInfo.apiVersion = XR_API_VERSION_1_0;
    strncpy(ci.applicationInfo.applicationName, "eyeballs",
            XR_MAX_APPLICATION_NAME_SIZE - 1);

    XrInstance instance = XR_NULL_HANDLE;
    XR_CHECK(xrCreateInstance(&ci, &instance));

    // Log runtime info
    XrInstanceProperties props{XR_TYPE_INSTANCE_PROPERTIES};
    if (XR_SUCCEEDED(xrGetInstanceProperties(instance, &props)))
        LOG("XR runtime: %s (ver %u.%u.%u)", props.runtimeName,
            XR_VERSION_MAJOR(props.runtimeVersion),
            XR_VERSION_MINOR(props.runtimeVersion),
            XR_VERSION_PATCH(props.runtimeVersion));

    LOG("xrCreateInstance: success");
    return instance;
}
