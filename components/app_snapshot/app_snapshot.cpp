// Renders one frame matching the main app pipeline exactly:
// same EGL setup, MSAA resolve, draw order, sky/cube/water/text, and
// Quest 3 asymmetric per-eye FOV via vr_math::projection.
// Outputs a side-by-side stereo PNG (left eye | right eye).
//
// Usage: app_snapshot [out.png] [time_sec]

// Must precede any openxr.h inclusion in this TU.
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_OPENGL_ES

// Pull in stbi_write_png declaration; implementation is in scene_snapshot.
#include "stb_image_write.h"

#include "host_gl_context.hpp"
#include "vr_math.hpp"
#include "scene.hpp"
#include "cube_mesh.hpp"
#include "text_mesh.hpp"
#include "water_surface.hpp"
#include "sky_dome.hpp"

#include <GLES3/gl3.h>
#include <Eigen/Core>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>

namespace {

// Per-eye resolution (Quest 3 is 2064x2208; scaled for snapshot convenience)
constexpr int kW = 960;
constexpr int kH = 1080;
// MSAA sample count: matches Quest 3 recommendedSwapchainSampleCount
constexpr GLsizei kSamples = 4;

// Near/far matching renderer.cpp
constexpr float kNear = 0.05f;
constexpr float kFar  = 100.0f;

// Approximate Quest 3 per-eye asymmetric FOV (inward bias)
constexpr float kD = float(M_PI) / 180.0f;
constexpr XrFovf kFovL = { -52.0f*kD,  40.0f*kD,  45.0f*kD, -50.0f*kD };
constexpr XrFovf kFovR = { -40.0f*kD,  52.0f*kD,  45.0f*kD, -50.0f*kD };

// Viewer pose: standing at origin, eyes at 1.6 m height, looking -Z
constexpr float kEyeHeight = 1.6f;
// Quest 3 default IPD
constexpr float kIpd = 0.063f;

// Water grid matching app.cpp
constexpr float kWaterY   = -1.5f;
constexpr float kCellSize = 0.375f;
constexpr int   kGridW    = 64;
constexpr int   kGridH    = 64;

struct EyeFbo {
    GLuint msaa_color = 0, msaa_depth = 0, msaa_fbo = 0;
    GLuint resolve_rb = 0, resolve_depth = 0, resolve_fbo = 0;
};

static bool make_eye_fbo(EyeFbo& e) {
    glGenRenderbuffers(1, &e.msaa_color);
    glBindRenderbuffer(GL_RENDERBUFFER, e.msaa_color);
    // GL_SRGB8_ALPHA8 matches choose_swapchain_format's preference, which is what
    // the XR swapchain uses. MSAA and resolve must share the same format.
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, kSamples, GL_SRGB8_ALPHA8, kW, kH);

    glGenRenderbuffers(1, &e.msaa_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, e.msaa_depth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, kSamples, GL_DEPTH_COMPONENT24, kW, kH);

    glGenFramebuffers(1, &e.msaa_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, e.msaa_fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, e.msaa_color);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, e.msaa_depth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "app_snapshot: msaa_fbo incomplete\n"); return false;
    }

    glGenRenderbuffers(1, &e.resolve_rb);
    glBindRenderbuffer(GL_RENDERBUFFER, e.resolve_rb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_SRGB8_ALPHA8, kW, kH);

    glGenRenderbuffers(1, &e.resolve_depth);
    glBindRenderbuffer(GL_RENDERBUFFER, e.resolve_depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kW, kH);

    glGenFramebuffers(1, &e.resolve_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, e.resolve_fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, e.resolve_rb);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_RENDERBUFFER, e.resolve_depth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "app_snapshot: resolve_fbo incomplete\n"); return false;
    }
    return true;
}

static void free_eye_fbo(EyeFbo& e) {
    glDeleteFramebuffers(1, &e.msaa_fbo);
    glDeleteRenderbuffers(1, &e.msaa_color);
    glDeleteRenderbuffers(1, &e.msaa_depth);
    glDeleteFramebuffers(1, &e.resolve_fbo);
    glDeleteRenderbuffers(1, &e.resolve_rb);
    glDeleteRenderbuffers(1, &e.resolve_depth);
}

// View matrix for a viewer at (ipd_offset, kEyeHeight, 0) looking -Z
static Eigen::Matrix4f eye_view(float ipd_offset) {
    Eigen::Matrix4f v = Eigen::Matrix4f::Identity();
    v(0,3) = -ipd_offset;
    v(1,3) = -kEyeHeight;
    return v;
}

static Light apply_day_cycle(float t, WaterSurface& water, SkyDome& sky) {
    constexpr float kDayPeriod = 120.0f;
    constexpr float kPi = float(M_PI);
    float phase     = float(fmod(t + kDayPeriod * 0.25f, kDayPeriod) / kDayPeriod);
    float elev_norm = sinf(2.0f * kPi * phase);
    float elev_rad  = elev_norm * (kPi / 2.0f);
    float az        = kPi * phase;
    float ce        = cosf(elev_rad);
    Eigen::Vector3f sun_dir{-ce * cosf(az), -sinf(elev_rad), -ce * sinf(az)};
    float warm = std::max(0.0f, 1.0f - fabsf(elev_norm) * 3.5f);
    warm *= warm;
    Eigen::Vector3f sun_color{1.0f, 0.85f + 0.15f*(1.0f-warm), 0.65f + 0.35f*(1.0f-warm)};
    float sun_intensity = std::max(0.05f, sinf(elev_rad) * 1.3f);

    SkyParams sp;
    sp.radius              = 500.0f;
    sp.sun_elevation       = elev_norm;
    sp.body_dir            = -sun_dir;
    sp.body_color          = {sun_color.x(), sun_color.y(), sun_color.z(), 1.0f};
    sp.body_angular_radius = 0.014f;
    sky.set_params(sp);

    Light sun;
    sun.direction = sun_dir;
    sun.color     = sun_color;
    sun.intensity = sun_intensity;
    water.set_sun(sun);
    return sun;
}

} // namespace

int main(int argc, char** argv) {
    const char* out_path = argc > 1 ? argv[1] : "/tmp/app_snapshot.png";
    float time_sec       = argc > 2 ? std::stof(argv[2]) : 0.0f;

    auto ctx = HostGlContext::create();
    if (!ctx) { fprintf(stderr, "app_snapshot: failed to create GL context\n"); return 1; }

    // Scene setup matching app.cpp
    Scene scene;
    scene.set_controller_poses(std::nullopt, std::nullopt);
    scene.update(static_cast<double>(time_sec));

    CubeMesh cube_mesh;
    cube_mesh.init();

    TextMesh text_mesh;
    text_mesh.init();

    WaterParams wp;
    wp.grid_w    = kGridW;
    wp.grid_h    = kGridH;
    wp.cell_size = kCellSize;
    WaterSurface water = WaterSurface::create(wp);
    water.update(time_sec);

    SkyDome sky = SkyDome::create({});
    Light sun_light = apply_day_cycle(time_sec, water, sky);

    // Water world transform matching app.cpp
    Eigen::Matrix4f water_model = Eigen::Matrix4f::Identity();
    water_model(0,3) = -(kGridW - 1) * kCellSize * 0.5f;
    water_model(1,3) = kWaterY;
    water_model(2,3) = -(kGridH - 1) * kCellSize * 0.5f;

    // Build per-eye FBOs (MSAA + resolve, matching eye_swapchain.cpp)
    EyeFbo eyes[2]{};
    if (!make_eye_fbo(eyes[0]) || !make_eye_fbo(eyes[1])) { return 1; }

    const XrFovf fovs[2]    = { kFovL, kFovR };
    const float  offsets[2] = { -kIpd * 0.5f, kIpd * 0.5f };

    std::vector<uint8_t> pixels[2];
    for (int ei = 0; ei < 2; ++ei) {
        EyeFbo& e    = eyes[ei];
        Eigen::Matrix4f proj     = projection(fovs[ei], kNear, kFar);
        Eigen::Matrix4f view_mat = eye_view(offsets[ei]);
        Eigen::Matrix4f pv       = proj * view_mat;
        Eigen::Vector3f view_pos = -(view_mat.block<3,3>(0,0).transpose()
                                     * view_mat.block<3,1>(0,3));

        // Draw into MSAA FBO (matches renderer.cpp render_eyes body)
        glBindFramebuffer(GL_FRAMEBUFFER, e.msaa_fbo);
        glViewport(0, 0, kW, kH);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDepthMask(GL_FALSE);
        sky.draw(pv);
        glDepthMask(GL_TRUE);

        cube_mesh.begin_batch({&sun_light, 1}, view_pos);
        for (const auto& cube : scene.cubes())
            cube_mesh.draw(pv * cube.model, cube.model, cube.material);
        CubeMesh::end_batch();

        for (const auto& lbl : scene.labels())
            text_mesh.draw(lbl.text, pv * lbl.transform);

        water.draw(pv * water_model, water_model, view_pos);

        // MSAA resolve — GL_NEAREST matches renderer.cpp comment
        glBindFramebuffer(GL_READ_FRAMEBUFFER, e.msaa_fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, e.resolve_fbo);
        glBlitFramebuffer(0, 0, kW, kH, 0, 0, kW, kH, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, e.resolve_fbo);
        pixels[ei].resize(kW * kH * 4);
        glReadPixels(0, 0, kW, kH, GL_RGBA, GL_UNSIGNED_BYTE, pixels[ei].data());
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    free_eye_fbo(eyes[0]);
    free_eye_fbo(eyes[1]);

    // Combine eyes side-by-side, flip rows (GL origin is bottom-left)
    constexpr int outW = kW * 2;
    constexpr int outH = kH;
    std::vector<uint8_t> out(outW * outH * 4);
    for (int ei = 0; ei < 2; ++ei) {
        const uint8_t* src   = pixels[ei].data();
        int            x_off = ei * kW;
        for (int row = 0; row < kH; ++row) {
            const uint8_t* s = src   + (kH - 1 - row) * kW * 4;
            uint8_t*       d = out.data() + row * outW * 4 + x_off * 4;
            std::copy(s, s + kW * 4, d);
        }
    }

    if (!stbi_write_png(out_path, outW, outH, 4, out.data(), outW * 4)) {
        fprintf(stderr, "app_snapshot: stbi_write_png failed for %s\n", out_path);
        return 1;
    }
    printf("Wrote %s (%dx%d stereo)\n", out_path, outW, outH);
    return 0;
}
