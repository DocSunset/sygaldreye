#include <android/log.h>
#include <android_native_app_glue.h>
#include <openxr/openxr.h>
#include "renderer.hpp"
#include "xr_session.hpp"
#include "frame_loop.hpp"
#include "scene.hpp"
#include "input.hpp"
#include "cube_mesh.hpp"
#include "text_mesh.hpp"
#include "water_surface.hpp"
#include "sky_dome.hpp"
#include "light.hpp"
#include <cmath>
#include <optional>

#define LOG(...)  __android_log_print(ANDROID_LOG_INFO,  "eyeballs", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,  "eyeballs", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "eyeballs", __VA_ARGS__)

XrInstance xr_create_instance(struct android_app*);
XrSystemId xr_get_system(XrInstance);

struct AppState {
    bool hasWindow = false;
    bool hasFocus  = false;
    bool renderable() const { return hasWindow && hasFocus; }
    XrInstance xrInstance = XR_NULL_HANDLE;
    XrSystemId xrSystemId = XR_NULL_SYSTEM_ID;
    Renderer renderer{};
    XrSessionObj xrSession{};
    FrameLoop    frame_loop_{};
    Scene scene_{};
    Input input_{};
    CubeMesh cube_mesh_{};
    TextMesh text_mesh_{};
    std::optional<WaterSurface> water_{};
    std::optional<SkyDome>      sky_{};
};

static void onAppCmd(struct android_app* app, int32_t cmd) {
    auto* s = static_cast<AppState*>(app->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:  s->hasWindow = true;  LOG("INIT_WINDOW");   break;
        case APP_CMD_TERM_WINDOW:  s->hasWindow = false; LOG("TERM_WINDOW");   break;
        case APP_CMD_GAINED_FOCUS: s->hasFocus  = true;  LOG("GAINED_FOCUS");  break;
        case APP_CMD_LOST_FOCUS:   s->hasFocus  = false; LOG("LOST_FOCUS");    break;
        case APP_CMD_DESTROY:                            LOG("DESTROY");       break;
    }
}

void android_main(struct android_app* app) {
    LOG("android_main: started");

    AppState state;
    state.xrInstance = xr_create_instance(app);
    state.xrSystemId = xr_get_system(state.xrInstance);
    auto binding = state.renderer.init();
    if (!binding) { LOGE("renderer init failed"); return; }
    state.xrSession.create(state.xrInstance, state.xrSystemId, &binding->xr_binding);
    state.renderer.create_swapchains(state.xrInstance, state.xrSystemId, state.xrSession.get());
    state.cube_mesh_.init();
    state.text_mesh_.init();
    state.input_.create(state.xrInstance, state.xrSession.get());

    WaterParams wp;
    wp.grid_w    = 64;
    wp.grid_h    = 64;
    wp.cell_size = 0.375f;
    state.water_ = WaterSurface::create(wp);

    SkyParams sp;
    sp.radius      = 500.0f;
    sp.star_count  = 2000;
    state.sky_ = SkyDome::create(sp);
    app->userData  = &state;
    app->onAppCmd  = onAppCmd;

    while (!app->destroyRequested && !state.xrSession.should_quit()) {
        int         events;
        android_poll_source* source;
        int timeout = state.xrSession.should_render() ? 0 : 10;
        if (ALooper_pollOnce(timeout, nullptr, &events,
                             reinterpret_cast<void**>(&source)) >= 0) {
            if (source) { source->process(app, source); }
        }
        if (!state.xrSession.poll_events()) {
            LOGE("poll_events failed — exiting loop");
            break;
        }
        if (state.xrSession.session_running()) {
            state.frame_loop_.run_frame(state.xrSession.get(), [&](XrTime t) -> FrameLayers {
                double time_sec = static_cast<double>(t) * 1e-9;
                if (!state.input_.sync(state.xrSession.get(), state.xrSession.worldSpace_(), t,
                                       state.xrSession.should_render())) {
                    LOGW("input sync failed — skipping input");
                }

                auto lh = state.input_.hand_pose(Hand::LEFT);
                auto rh = state.input_.hand_pose(Hand::RIGHT);
                state.scene_.update(time_sec);
                state.scene_.set_controller_poses(
                    lh ? std::optional<XrPosef>{lh->pose} : std::nullopt,
                    rh ? std::optional<XrPosef>{rh->pose} : std::nullopt);

                // Day-night cycle: 180-second period
                constexpr double kDayPeriod = 180.0;
                // +0.25 offsets the cycle so time 0 lands at solar noon (elev_norm = 1)
                float phase      = static_cast<float>(fmod(time_sec + kDayPeriod * 0.25, kDayPeriod) / kDayPeriod);
                float elev_norm  = sinf(2.0f * static_cast<float>(M_PI) * phase); // -1..1
                float elev_rad   = elev_norm * (static_cast<float>(M_PI) / 2.0f);
                float azimuth    = static_cast<float>(M_PI) * phase;
                float ce         = cosf(elev_rad);
                Eigen::Vector3f sun_dir{-ce * cosf(azimuth), -sinf(elev_rad), -ce * sinf(azimuth)};
                float warm       = std::max(0.0f, 1.0f - fabsf(elev_norm) * 3.5f);
                warm = warm * warm;
                Eigen::Vector3f sun_color{1.0f, 0.85f + 0.15f * (1.0f - warm), 0.65f + 0.35f * (1.0f - warm)};
                float sun_intensity = std::max(0.05f, sinf(elev_rad) * 1.3f);

                if (state.sky_) {
                    SkyParams sp;
                    sp.radius              = 500.0f;
                    sp.star_count          = 2000;
                    sp.sun_elevation       = elev_norm;
                    sp.body_dir            = -sun_dir;
                    sp.body_color          = {sun_color.x(), sun_color.y(), sun_color.z(), 1.0f};
                    sp.body_angular_radius = 0.014f;
                    state.sky_->set_params(sp);
                }

                Light sun_light;
                sun_light.direction = sun_dir;
                sun_light.color     = sun_color;
                sun_light.intensity = sun_intensity;
                if (state.water_) {
                    state.water_->set_sun(sun_light);
                    state.water_->update(static_cast<float>(time_sec));
                }

                // Water model: centered at user, 1.5m below floor
                constexpr float kWaterY    = -1.5f;
                constexpr float kCellSize  = 0.375f;
                constexpr int   kGridW     = 64;
                constexpr int   kGridH     = 64;
                Eigen::Matrix4f water_model = Eigen::Matrix4f::Identity();
                water_model(0,3) = -(kGridW - 1) * kCellSize * 0.5f;
                water_model(1,3) = kWaterY;
                water_model(2,3) = -(kGridH - 1) * kCellSize * 0.5f;

                bool ok = state.renderer.render_eyes(
                    state.xrInstance, state.xrSession.get(),
                    state.xrSession.worldSpace_(), t,
                    [&](const Eigen::Matrix4f& proj, const Eigen::Matrix4f& view) {
                        const Eigen::Matrix4f pv = proj * view;
                        const Eigen::Vector3f view_pos =
                            -(view.block<3,3>(0,0).transpose() * view.block<3,1>(0,3));

                        // Sky dome first (no depth write, always behind everything)
                        if (state.sky_) {
                            glDepthMask(GL_FALSE);
                            state.sky_->draw(pv);
                            glDepthMask(GL_TRUE);
                        }

                        state.cube_mesh_.begin_batch({&sun_light, 1}, view_pos);
                        for (const auto& cube : state.scene_.cubes()) {
                            state.cube_mesh_.draw(pv * cube.model, cube.model, cube.material);
                        }
                        state.cube_mesh_.end_batch();
                        for (const auto& lbl : state.scene_.labels()) {
                            state.text_mesh_.draw(lbl.text, pv * lbl.transform);
                        }

                        if (state.water_) {
                            state.water_->draw(pv * water_model, water_model, view_pos);
                        }
                    });
                if (!ok) { return {}; }
                FrameLayers result;
                result.layers.at(0) = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&state.renderer.proj_layer());
                result.count = 1;
                return result;
            });
        }
    }

    LOG("android_main: exiting");
}
