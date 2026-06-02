#include <android/log.h>
#include <android_native_app_glue.h>
#include <openxr/openxr.h>
#include <vector>
#include "renderer.hpp"
#include "xr_session.hpp"
#include "scene.hpp"
#include "input.hpp"
#include "vr_math.hpp"

#define LOG(...) __android_log_print(ANDROID_LOG_INFO, "eyeballs", __VA_ARGS__)

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
    Scene scene_{};
    Input input_{};
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
    state.renderer.init();
    auto binding = state.renderer.graphics_binding();
    state.xrSession.create(state.xrInstance, state.xrSystemId, &binding);
    state.renderer.create_swapchains(state.xrInstance, state.xrSystemId, state.xrSession.get());
    state.input_.create(state.xrInstance, state.xrSession.get());
    app->userData  = &state;
    app->onAppCmd  = onAppCmd;

    while (!app->destroyRequested && !state.xrSession.should_quit()) {
        int         events;
        android_poll_source* source;
        int timeout = state.xrSession.should_render() ? 0 : 10;
        if (ALooper_pollOnce(timeout, nullptr, &events,
                             reinterpret_cast<void**>(&source)) >= 0) {
            if (source) source->process(app, source);
        }
        state.xrSession.poll_events();
        if (state.xrSession.session_running()) {
            state.xrSession.render_frame([&](XrTime t) -> std::vector<const XrCompositionLayerBaseHeader*> {
                double time_sec = static_cast<double>(t) * 1e-9;
                state.input_.sync(state.xrSession.get(), state.xrSession.worldSpace_(), t);

                // Hand cube tuning — edit these to adjust placement (in cm)
                constexpr float HAND_CUBE_OFFSET_X_CM =  0.0f;
                constexpr float HAND_CUBE_OFFSET_Y_CM =  0.0f;  // up in grip frame
                constexpr float HAND_CUBE_OFFSET_Z_CM =  0.0f;

                Eigen::Matrix4f scale_m = Eigen::Matrix4f::Identity();
                scale_m(0,0) = scale_m(1,1) = scale_m(2,2) = 0.035f;

                Eigen::Matrix4f local_T = Eigen::Matrix4f::Identity();
                local_T(0,3) = HAND_CUBE_OFFSET_X_CM * 0.01f;
                local_T(1,3) = HAND_CUBE_OFFSET_Y_CM * 0.01f;
                local_T(2,3) = HAND_CUBE_OFFSET_Z_CM * 0.01f;

                HandPose lh = state.input_.hand_pose(0);
                HandPose rh = state.input_.hand_pose(1);
                Eigen::Matrix4f lm = lh.valid ? (pose_to_world(lh.pose) * local_T * scale_m).eval() : Eigen::Matrix4f::Identity();
                Eigen::Matrix4f rm = rh.valid ? (pose_to_world(rh.pose) * local_T * scale_m).eval() : Eigen::Matrix4f::Identity();

                state.scene_.update(time_sec);
                state.scene_.set_controller_poses(&lm, lh.valid, &rm, rh.valid);
                bool ok = state.renderer.render_eyes(
                    state.xrInstance, state.xrSession.get(),
                    state.xrSession.worldSpace_(), t,
                    state.scene_.cubes());
                if (!ok) return {};
                return { reinterpret_cast<const XrCompositionLayerBaseHeader*>(&state.renderer.proj_layer()) };
            });
        }
    }

    LOG("android_main: exiting");
}
