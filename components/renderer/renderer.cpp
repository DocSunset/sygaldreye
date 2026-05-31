#include "renderer.hpp"
#include <android/log.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

#define XR_CHECK(r) do { XrResult _r = (r); if (XR_FAILED(_r)) { LOGE(#r " failed: %d", (int)_r); return false; } } while(0)

bool Renderer::init() {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) { LOGE("eglGetDisplay failed"); return false; }

    if (!eglInitialize(display, nullptr, nullptr)) {
        LOGE("eglInitialize failed: 0x%x", eglGetError()); return false;
    }

    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE,    EGL_PBUFFER_BIT,
        EGL_RED_SIZE,   8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0,
        EGL_NONE
    };
    EGLint numConfigs = 0;
    if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs) || numConfigs < 1) {
        LOGE("eglChooseConfig failed: 0x%x", eglGetError()); return false;
    }

    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed: 0x%x", eglGetError()); return false;
    }

    const EGLint pbAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    surface = eglCreatePbufferSurface(display, config, pbAttribs);
    if (surface == EGL_NO_SURFACE) {
        LOGE("eglCreatePbufferSurface failed: 0x%x", eglGetError()); return false;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOGE("eglMakeCurrent failed: 0x%x", eglGetError()); return false;
    }

    LOG("eglMakeCurrent success");
    LOG("GL_VERSION:  %s", glGetString(GL_VERSION));
    LOG("GL_RENDERER: %s", glGetString(GL_RENDERER));
    LOG("GL_VENDOR:   %s", glGetString(GL_VENDOR));
    return true;
}

XrGraphicsBindingOpenGLESAndroidKHR Renderer::graphics_binding() const {
    return {
        .type    = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
        .next    = nullptr,
        .display = display,
        .config  = config,
        .context = context,
    };
}

bool Renderer::create_swapchains(XrInstance instance, XrSystemId systemId, XrSession session) {
    // enumerate view configuration views
    uint32_t viewCount = 0;
    XR_CHECK(xrEnumerateViewConfigurationViews(instance, systemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewCount, nullptr));
    if (viewCount != 2) { LOGE("expected 2 views, got %u", viewCount); return false; }

    XrViewConfigurationView vcv[2]{};
    vcv[0].type = vcv[1].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
    XR_CHECK(xrEnumerateViewConfigurationViews(instance, systemId,
        XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 2, &viewCount, vcv));

    LOG("view count: %u", viewCount);
    LOG("eye[0] recommended: %ux%u samples=%u",
        vcv[0].recommendedImageRectWidth, vcv[0].recommendedImageRectHeight,
        vcv[0].recommendedSwapchainSampleCount);
    LOG("eye[1] recommended: %ux%u samples=%u",
        vcv[1].recommendedImageRectWidth, vcv[1].recommendedImageRectHeight,
        vcv[1].recommendedSwapchainSampleCount);

    // choose swapchain format
    uint32_t fmtCount = 0;
    XR_CHECK(xrEnumerateSwapchainFormats(session, 0, &fmtCount, nullptr));
    std::vector<int64_t> fmts(fmtCount);
    XR_CHECK(xrEnumerateSwapchainFormats(session, fmtCount, &fmtCount, fmts.data()));

    int64_t chosenFmt = 0;
    for (int64_t f : fmts) if (f == GL_SRGB8_ALPHA8) { chosenFmt = f; break; }
    if (!chosenFmt) for (int64_t f : fmts) if (f == GL_RGBA8) { chosenFmt = f; break; }
    if (!chosenFmt) { chosenFmt = fmts[0]; }
    LOG("swapchain format: 0x%llx", (unsigned long long)chosenFmt);

    for (int eye = 0; eye < 2; ++eye) {
        EyeSwapchain& e = eyes[eye];
        e.width  = vcv[eye].recommendedImageRectWidth;
        e.height = vcv[eye].recommendedImageRectHeight;

        XrSwapchainCreateInfo sci{XR_TYPE_SWAPCHAIN_CREATE_INFO};
        sci.usageFlags  = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        sci.format      = chosenFmt;
        sci.sampleCount = 1;
        sci.width       = e.width;
        sci.height      = e.height;
        sci.faceCount   = 1;
        sci.arraySize   = 1;
        sci.mipCount    = 1;
        XR_CHECK(xrCreateSwapchain(session, &sci, &e.handle));
        LOG("eye[%d] swapchain created", eye);

        uint32_t imgCount = 0;
        XR_CHECK(xrEnumerateSwapchainImages(e.handle, 0, &imgCount, nullptr));
        e.images.resize(imgCount, {XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR});
        XR_CHECK(xrEnumerateSwapchainImages(e.handle, imgCount, &imgCount,
            reinterpret_cast<XrSwapchainImageBaseHeader*>(e.images.data())));

        e.fbos.resize(imgCount);
        glGenFramebuffers((GLsizei)imgCount, e.fbos.data());
        for (uint32_t i = 0; i < imgCount; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, e.fbos[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, e.images[i].image, 0);
            if (i == 0) {
                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                if (status == GL_FRAMEBUFFER_COMPLETE)
                    LOG("eye[%d] framebuffer complete", eye);
                else
                    LOGE("eye[%d] framebuffer status: 0x%x", eye, status);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        LOG("eye[%d] fbos: %u", eye, imgCount);
    }
    return true;
}
