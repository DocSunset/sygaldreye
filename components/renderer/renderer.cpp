#include "renderer.hpp"
#include "cube_mesh.hpp"
#include "vr_math.hpp"
#include <android/log.h>
#include <time.h>
#include <utility>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

#define XR_CHECK(r) do { XrResult _r = (r); if (XR_FAILED(_r)) { LOGE(#r " failed: %d", (int)_r); return false; } } while(0)
#define XR_LOG_ERR(expr) do { XrResult _r = (expr); if (XR_FAILED(_r)) LOGE(#expr " failed: %d", (int)_r); } while(0)

namespace {
constexpr float kNearPlane = 0.05F;
constexpr float kFarPlane  = 100.0F;
}

static double now_sec() {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

// ── EyeSwapchain ──────────────────────────────────────────────────────────────

EyeSwapchain::~EyeSwapchain() {
    if (!fbos.empty()) {
        glDeleteFramebuffers(static_cast<GLsizei>(fbos.size()), fbos.data());
    }
    if (!depth_rbs.empty()) {
        glDeleteRenderbuffers(static_cast<GLsizei>(depth_rbs.size()), depth_rbs.data());
    }
    if (handle != XR_NULL_HANDLE) {
        XR_LOG_ERR(xrDestroySwapchain(handle));
    }
}

EyeSwapchain::EyeSwapchain(EyeSwapchain&& other) noexcept
    : handle(std::exchange(other.handle, XR_NULL_HANDLE))
    , width(std::exchange(other.width, 0U))
    , height(std::exchange(other.height, 0U))
    , images(std::move(other.images))
    , fbos(std::move(other.fbos))
    , depth_rbs(std::move(other.depth_rbs))
{}

EyeSwapchain& EyeSwapchain::operator=(EyeSwapchain&& other) noexcept {
    if (this != &other) {
        this->~EyeSwapchain();
        handle    = std::exchange(other.handle, XR_NULL_HANDLE);
        width     = std::exchange(other.width, 0U);
        height    = std::exchange(other.height, 0U);
        images    = std::move(other.images);
        fbos      = std::move(other.fbos);
        depth_rbs = std::move(other.depth_rbs);
    }
    return *this;
}

// ── Renderer ──────────────────────────────────────────────────────────────────

Renderer::Renderer() = default;

Renderer::~Renderer() {
    if (surface != EGL_NO_SURFACE) {
        eglDestroySurface(display, surface);
    }
    if (context != EGL_NO_CONTEXT) {
        eglDestroyContext(display, context);
    }
    if (display != EGL_NO_DISPLAY) {
        eglTerminate(display);
    }
}

Renderer::Renderer(Renderer&& other) noexcept
    : display(std::exchange(other.display, EGL_NO_DISPLAY))
    , config(std::exchange(other.config, nullptr))
    , context(std::exchange(other.context, EGL_NO_CONTEXT))
    , surface(std::exchange(other.surface, EGL_NO_SURFACE))
    , eyes(std::move(other.eyes))
    , projViews(other.projViews)
    , projLayer(other.projLayer)
    , cube_mesh_(std::move(other.cube_mesh_))
    , firstEyeRender_(std::exchange(other.firstEyeRender_, true))
    , layerLogged_(std::exchange(other.layerLogged_, false))
    , lastLocateErr_(std::exchange(other.lastLocateErr_, 0.0))
{}


Renderer& Renderer::operator=(Renderer&& other) noexcept {
    if (this != &other) {
        this->~Renderer();
        display    = std::exchange(other.display, EGL_NO_DISPLAY);
        config     = std::exchange(other.config, nullptr);
        context    = std::exchange(other.context, EGL_NO_CONTEXT);
        surface    = std::exchange(other.surface, EGL_NO_SURFACE);
        eyes       = std::move(other.eyes);
        projViews       = other.projViews;
        projLayer       = other.projLayer;
        cube_mesh_      = std::move(other.cube_mesh_);
        firstEyeRender_ = std::exchange(other.firstEyeRender_, true);
        layerLogged_    = std::exchange(other.layerLogged_, false);
        lastLocateErr_  = std::exchange(other.lastLocateErr_, 0.0);
    }
    return *this;
}

std::optional<RendererBinding> Renderer::init() {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) { LOGE("eglGetDisplay failed"); return std::nullopt; }

    if (!eglInitialize(display, nullptr, nullptr)) {
        LOGE("eglInitialize failed: 0x%x", eglGetError()); return std::nullopt;
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
        LOGE("eglChooseConfig failed: 0x%x", eglGetError()); return std::nullopt;
    }

    const EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (context == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed: 0x%x", eglGetError()); return std::nullopt;
    }

    const EGLint pbAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    surface = eglCreatePbufferSurface(display, config, pbAttribs);
    if (surface == EGL_NO_SURFACE) {
        LOGE("eglCreatePbufferSurface failed: 0x%x", eglGetError()); return std::nullopt;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
        LOGE("eglMakeCurrent failed: 0x%x", eglGetError()); return std::nullopt;
    }

    LOG("eglMakeCurrent success");
    LOG("GL_VERSION:  %s", glGetString(GL_VERSION));
    LOG("GL_RENDERER: %s", glGetString(GL_RENDERER));
    LOG("GL_VENDOR:   %s", glGetString(GL_VENDOR));
    return RendererBinding{
        .xr_binding = {
            .type    = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR,
            .next    = nullptr,
            .display = display,
            .config  = config,
            .context = context,
        }
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
        e.depth_rbs.resize(imgCount);
        glGenFramebuffers((GLsizei)imgCount, e.fbos.data());
        glGenRenderbuffers((GLsizei)imgCount, e.depth_rbs.data());
        for (uint32_t i = 0; i < imgCount; ++i) {
            glBindRenderbuffer(GL_RENDERBUFFER, e.depth_rbs[i]);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, (GLsizei)e.width, (GLsizei)e.height);

            glBindFramebuffer(GL_FRAMEBUFFER, e.fbos[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, e.images[i].image, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                GL_RENDERBUFFER, e.depth_rbs[i]);

            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status == GL_FRAMEBUFFER_COMPLETE)
                LOG("eye[%d] fbo[%u] framebuffer complete", eye, i);
            else
                LOGE("eye[%d] fbo[%u] framebuffer status: 0x%x", eye, i, status);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        LOG("eye[%d] fbos: %u", eye, imgCount);
    }
    cube_mesh_ = std::make_unique<CubeMesh>();
    cube_mesh_->init();
    LOG("cube_mesh initialized");
    return true;
}

bool Renderer::render_eyes(XrInstance instance, XrSession session, XrSpace refSpace,
                           XrTime predictedDisplayTime, std::span<const CubeInstance> cubes) {
    if (firstEyeRender_) { LOG("eye rendering started"); firstEyeRender_ = false; }

    double t = now_sec();

    // Locate views BEFORE rendering so we have eye poses for MVP computation
    XrViewLocateInfo vli{XR_TYPE_VIEW_LOCATE_INFO};
    vli.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    vli.displayTime           = predictedDisplayTime;
    vli.space                 = refSpace;

    XrViewState vs{XR_TYPE_VIEW_STATE};
    XrView views[2]{{XR_TYPE_VIEW},{XR_TYPE_VIEW}};
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

    for (int eye = 0; eye < 2; ++eye) {
        EyeSwapchain& e = eyes[eye];

        XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
        uint32_t index = 0;
        XR_LOG_ERR(xrAcquireSwapchainImage(e.handle, &ai, &index));

        XrSwapchainImageWaitInfo wi{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
        wi.timeout = XR_INFINITE_DURATION;
        XR_LOG_ERR(xrWaitSwapchainImage(e.handle, &wi));

        glBindFramebuffer(GL_FRAMEBUFFER, e.fbo(index));
        glViewport(0, 0, (GLsizei)e.width, (GLsizei)e.height);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Eigen::Matrix4f proj = projection(views[eye].fov, kNearPlane, kFarPlane);
        Eigen::Matrix4f v    = view(views[eye].pose);
        for (const auto& cube : cubes) {
            Eigen::Matrix4f mvp = proj * v * cube.model;
            cube_mesh_->draw(mvp);
        }

        XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
        XR_LOG_ERR(xrReleaseSwapchainImage(e.handle, &ri));
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (!layerLogged_) { LOG("submitting projection layer"); layerLogged_ = true; }

    for (int eye = 0; eye < 2; ++eye) {
        projViews[eye] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
        projViews[eye].pose = views[eye].pose;
        projViews[eye].fov  = views[eye].fov;
        projViews[eye].subImage.swapchain        = eyes[eye].handle;
        projViews[eye].subImage.imageRect        = {{0, 0}, {(int32_t)eyes[eye].width, (int32_t)eyes[eye].height}};
        projViews[eye].subImage.imageArrayIndex  = 0;
    }

    projLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    projLayer.space      = refSpace;
    projLayer.layerFlags = 0;
    projLayer.viewCount  = 2;
    projLayer.views      = projViews.data();

    return true;
}
