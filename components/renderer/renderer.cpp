#include "renderer.hpp"
#include <android/log.h>
#include <time.h>

#define LOG(...)  __android_log_print(ANDROID_LOG_INFO,  "eyeballs", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

#define XR_LOG_ERR(expr) do { XrResult _r = (expr); if (XR_FAILED(_r)) LOGE(#expr " failed: %d", (int)_r); } while(0)

namespace {
constexpr float      kNearPlane              = 0.05F;
constexpr float      kFarPlane               = 100.0F;
constexpr XrDuration kSwapchainWaitTimeoutNs = XR_INFINITE_DURATION;
constexpr double     kFrameBudgetWarningMs   = 9.0;
}


static double now_sec() {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

std::optional<RendererBinding> Renderer::init() {
    return egl_.init();
}

bool Renderer::create_swapchains(XrInstance instance, XrSystemId systemId, XrSession session) {
    return ::create_swapchains(instance, systemId, session, eyes_);
}

bool Renderer::render_eyes(XrInstance /*instance*/, XrSession session, XrSpace refSpace,
                           XrTime predictedDisplayTime,
                           const std::function<void(const Eigen::Matrix4f& proj,
                                                    const Eigen::Matrix4f& view)>& on_draw) {
    if (firstEyeRender_) { LOG("eye rendering started"); firstEyeRender_ = false; }

    double t = now_sec();

    XrViewLocateInfo vli{};
    vli.type                  = XR_TYPE_VIEW_LOCATE_INFO;
    vli.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    vli.displayTime           = predictedDisplayTime;
    vli.space                 = refSpace;

    XrViewState vs{};
    vs.type = XR_TYPE_VIEW_STATE;
    XrView views[2]{};
    views[0].type = views[1].type = XR_TYPE_VIEW;
    uint32_t viewCount = 2;
    XrResult lr = xrLocateViews(session, &vli, &vs, 2, &viewCount, views);
    if (XR_FAILED(lr)) {
        if (t - lastLocateErr_ >= 2.0) {
            LOGE("xrLocateViews failed: %d", (int)lr);
            lastLocateErr_ = t;
        }
        return false;
    }

    const uint32_t orientValid = XR_VIEW_STATE_ORIENTATION_VALID_BIT;
    const uint32_t posValid    = XR_VIEW_STATE_POSITION_VALID_BIT;
    if (!(vs.viewStateFlags & orientValid) || !(vs.viewStateFlags & posValid))
        return false;

    double draw_start = 0.0;
    for (int eye = 0; eye < 2; ++eye) {
        EyeSwapchain& e = eyes_[eye];

        XrSwapchainImageAcquireInfo ai{};
        ai.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;
        uint32_t index = 0;
        if (XR_FAILED(xrAcquireSwapchainImage(e.xr_handle(), &ai, &index))) {
            LOGE("xrAcquireSwapchainImage failed eye %d", eye);
            continue;
        }
        XrSwapchainImageWaitInfo wi{};
        wi.type    = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
        wi.timeout = kSwapchainWaitTimeoutNs;
        XrSwapchainImageReleaseInfo ri{};
        ri.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;
        if (XR_FAILED(xrWaitSwapchainImage(e.xr_handle(), &wi))) {
            LOGE("xrWaitSwapchainImage timed out or failed eye %d", eye);
            XR_LOG_ERR(xrReleaseSwapchainImage(e.xr_handle(), &ri));
            continue;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, e.msaa_fbo());
        glViewport(0, 0, (GLsizei)e.width(), (GLsizei)e.height());
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (eye == 0) draw_start = now_sec();
        Eigen::Matrix4f proj = projection(views[eye].fov, kNearPlane, kFarPlane);
        Eigen::Matrix4f v    = view(views[eye].pose);
        on_draw(proj, v);
        { GLenum e_ = glGetError(); if (e_ != GL_NO_ERROR) LOGE("GL error after on_draw eye %d: 0x%x", eye, (unsigned)e_); }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, e.msaa_fbo());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, e.fbo(index));
        // MSAA resolve: must use GL_NEAREST — GL_LINEAR is INVALID_OPERATION with a multisampled source
        glBlitFramebuffer(0, 0, (GLint)e.width(), (GLint)e.height(),
                          0, 0, (GLint)e.width(), (GLint)e.height(),
                          GL_COLOR_BUFFER_BIT, GL_NEAREST);
        { GLenum e_ = glGetError(); if (e_ != GL_NO_ERROR) LOGE("glBlitFramebuffer error: 0x%x", (unsigned)e_); }

        if (eye == 0 && post_resolve_hook) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, e.fbo(index));
            post_resolve_hook((int)e.width(), (int)e.height());
        }

        XR_LOG_ERR(xrReleaseSwapchainImage(e.xr_handle(), &ri));
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (draw_start > 0.0) {
        double draw_ms = (now_sec() - draw_start) * 1000.0;
        if (draw_ms > kFrameBudgetWarningMs)
            LOGW("render over budget: %.2f ms", draw_ms);
    }

    if (!layerLogged_) { LOG("submitting projection layer"); layerLogged_ = true; }

    for (int eye = 0; eye < 2; ++eye) {
        projViews_[eye] = {};
        projViews_[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
        projViews_[eye].pose = views[eye].pose;
        projViews_[eye].fov  = views[eye].fov;
        projViews_[eye].subImage.swapchain       = eyes_[eye].xr_handle();
        projViews_[eye].subImage.imageRect       = {{0, 0}, {(int32_t)eyes_[eye].width(), (int32_t)eyes_[eye].height()}};
        projViews_[eye].subImage.imageArrayIndex = 0;
    }

    projLayer_ = {};
    projLayer_.type       = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
    projLayer_.space      = refSpace;
    projLayer_.layerFlags = 0;
    projLayer_.viewCount  = 2;
    projLayer_.views      = projViews_.data();

    return true;
}
