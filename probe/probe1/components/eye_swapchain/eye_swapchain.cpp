#include "eye_swapchain.hpp"
#include <android/log.h>
#include <utility>

#define LOG(...)  __android_log_print(ANDROID_LOG_INFO,  "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

#define XR_CHECK(r) do { XrResult _r = (r); if (XR_FAILED(_r)) { LOGE(#r " failed: %d", (int)_r); return false; } } while(0)
#define XR_LOG_ERR(expr) do { XrResult _r = (expr); if (XR_FAILED(_r)) LOGE(#expr " failed: %d", (int)_r); } while(0)

#define GL_CHECK(call) do { \
    (call); \
    GLenum _gl_err = glGetError(); \
    if (_gl_err != GL_NO_ERROR) { \
        LOGE("GL error 0x%x in " #call " (" __FILE__ ":%d)", (unsigned)_gl_err, __LINE__); \
    } \
} while(0)

int64_t choose_swapchain_format(std::span<const int64_t> formats) {
    if (formats.empty()) { return 0; }
    for (int64_t fmt : formats) { if (fmt == GL_SRGB8_ALPHA8) { return fmt; } }
    for (int64_t fmt : formats) { if (fmt == GL_RGBA8)        { return fmt; } }
    return formats[0];
}

EyeSwapchain::~EyeSwapchain() {
    if (!fbos.empty())      { glDeleteFramebuffers(static_cast<GLsizei>(fbos.size()), fbos.data()); }
    if (!depth_rbs.empty()) { glDeleteRenderbuffers(static_cast<GLsizei>(depth_rbs.size()), depth_rbs.data()); }
    if (msaa_fbo_ != 0U)       { glDeleteFramebuffers(1, &msaa_fbo_); }
    if (msaa_color_rb_ != 0U)  { glDeleteRenderbuffers(1, &msaa_color_rb_); }
    if (msaa_depth_rb_ != 0U)  { glDeleteRenderbuffers(1, &msaa_depth_rb_); }
    if (handle != XR_NULL_HANDLE) { XR_LOG_ERR(xrDestroySwapchain(handle)); }
}

EyeSwapchain::EyeSwapchain(EyeSwapchain&& other) noexcept
    : handle(std::exchange(other.handle, XR_NULL_HANDLE))
    , width_(std::exchange(other.width_, 0U))
    , height_(std::exchange(other.height_, 0U))
    , sample_count_(std::exchange(other.sample_count_, 1U))
    , msaa_color_rb_(std::exchange(other.msaa_color_rb_, 0U))
    , msaa_depth_rb_(std::exchange(other.msaa_depth_rb_, 0U))
    , msaa_fbo_(std::exchange(other.msaa_fbo_, 0U))
    , images(std::move(other.images))
    , fbos(std::move(other.fbos))
    , depth_rbs(std::move(other.depth_rbs))
{}

EyeSwapchain& EyeSwapchain::operator=(EyeSwapchain&& other) noexcept {
    if (this != &other) {
        this->~EyeSwapchain();
        handle        = std::exchange(other.handle, XR_NULL_HANDLE);
        width_        = std::exchange(other.width_, 0U);
        height_       = std::exchange(other.height_, 0U);
        sample_count_ = std::exchange(other.sample_count_, 1U);
        msaa_color_rb_ = std::exchange(other.msaa_color_rb_, 0U);
        msaa_depth_rb_ = std::exchange(other.msaa_depth_rb_, 0U);
        msaa_fbo_      = std::exchange(other.msaa_fbo_, 0U);
        images    = std::move(other.images);
        fbos      = std::move(other.fbos);
        depth_rbs = std::move(other.depth_rbs);
    }
    return *this;
}

bool create_swapchains(XrInstance instance, XrSystemId systemId, XrSession session,
                       std::array<EyeSwapchain, 2>& out) {
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

    uint32_t fmtCount = 0;
    XR_CHECK(xrEnumerateSwapchainFormats(session, 0, &fmtCount, nullptr));
    std::vector<int64_t> fmts(fmtCount);
    XR_CHECK(xrEnumerateSwapchainFormats(session, fmtCount, &fmtCount, fmts.data()));

    int64_t chosenFmt = choose_swapchain_format(fmts);
    LOG("swapchain format: 0x%llx", (unsigned long long)chosenFmt);

    for (int eye = 0; eye < 2; ++eye) {
        EyeSwapchain& e = out[eye];
        e.width_  = vcv[eye].recommendedImageRectWidth;
        e.height_ = vcv[eye].recommendedImageRectHeight;

        XrSwapchainCreateInfo sci{};
        sci.type        = XR_TYPE_SWAPCHAIN_CREATE_INFO;
        sci.usageFlags  = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        sci.format      = chosenFmt;
        sci.sampleCount = 1;
        sci.width       = e.width_;
        sci.height      = e.height_;
        sci.faceCount   = 1;
        sci.arraySize   = 1;
        sci.mipCount    = 1;
        XR_CHECK(xrCreateSwapchain(session, &sci, &e.handle));
        LOG("eye[%d] swapchain created", eye);

        uint32_t imgCount = 0;
        XR_CHECK(xrEnumerateSwapchainImages(e.handle, 0, &imgCount, nullptr));
        XrSwapchainImageOpenGLESKHR img{};
        img.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
        e.images.resize(imgCount, img);
        XR_CHECK(xrEnumerateSwapchainImages(e.handle, imgCount, &imgCount,
            reinterpret_cast<XrSwapchainImageBaseHeader*>(e.images.data())));

        e.fbos.resize(imgCount);
        e.depth_rbs.resize(imgCount);
        GL_CHECK(glGenFramebuffers((GLsizei)imgCount, e.fbos.data()));
        GL_CHECK(glGenRenderbuffers((GLsizei)imgCount, e.depth_rbs.data()));
        for (uint32_t i = 0; i < imgCount; ++i) {
            GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, e.depth_rbs[i]));
            GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                (GLsizei)e.width_, (GLsizei)e.height_));
            GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, e.fbos[i]));
            GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, e.images[i].image, 0));
            GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                GL_RENDERBUFFER, e.depth_rbs[i]));
        }

        e.sample_count_ = vcv[eye].recommendedSwapchainSampleCount;
        GL_CHECK(glGenRenderbuffers(1, &e.msaa_color_rb_));
        GL_CHECK(glGenRenderbuffers(1, &e.msaa_depth_rb_));
        GL_CHECK(glGenFramebuffers(1, &e.msaa_fbo_));
        GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, e.msaa_color_rb_));
        GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, (GLsizei)e.sample_count_,
            static_cast<GLenum>(chosenFmt), (GLsizei)e.width_, (GLsizei)e.height_));
        GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, e.msaa_depth_rb_));
        GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, (GLsizei)e.sample_count_,
            GL_DEPTH_COMPONENT24, (GLsizei)e.width_, (GLsizei)e.height_));
        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, e.msaa_fbo_));
        GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_RENDERBUFFER, e.msaa_color_rb_));
        GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_RENDERBUFFER, e.msaa_depth_rb_));
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status == GL_FRAMEBUFFER_COMPLETE)
            LOG("eye[%d] msaa_fbo complete (samples=%u)", eye, e.sample_count_);
        else
            LOGE("eye[%d] msaa_fbo status: 0x%x", eye, status);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
    return true;
}
