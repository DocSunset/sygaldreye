#pragma once
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES
#include <jni.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

struct XrSession_;  // forward not needed; XrSession is a handle

struct XrSessionObj {
    XrSession handle = XR_NULL_HANDLE;

    bool create(XrInstance, XrSystemId, const XrGraphicsBindingOpenGLESAndroidKHR&);
    XrSession get() const { return handle; }
};
