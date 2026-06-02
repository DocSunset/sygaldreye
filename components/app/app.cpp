#include <android/log.h>
#include <android_native_app_glue.h>
#include <openxr/openxr.h>
#include <vector>
#include "renderer.hpp"
#include "xr_session.hpp"
#include "scene.hpp"
#include "input.hpp"

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
    auto binding = state.renderer.init();
    if (!binding) { LOGE("renderer init failed"); return; }
    state.xrSession.create(state.xrInstance, state.xrSystemId, &binding->xr_binding);
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
            if (source) { source->process(app, source); }
        }
        if (!state.xrSession.poll_events()) {
            LOGE("poll_events failed — exiting loop");
            break;
        }
        if (state.xrSession.session_running()) {
            state.xrSession.render_frame([&](XrTime t) -> std::vector<const XrCompositionLayerBaseHeader*> {
                double time_sec = static_cast<double>(t) * 1e-9;
                if (!state.input_.sync(state.xrSession.get(), state.xrSession.worldSpace_(), t)) {
                    LOGW("input sync failed — skipping input");
                }

                auto lh = state.input_.hand_pose(Hand::LEFT);
                auto rh = state.input_.hand_pose(Hand::RIGHT);
                state.scene_.update(time_sec);
                state.scene_.set_controller_poses(
                    lh ? std::optional<XrPosef>{lh->pose} : std::nullopt,
                    rh ? std::optional<XrPosef>{rh->pose} : std::nullopt);
                bool ok = state.renderer.render_eyes(
                    state.xrInstance, state.xrSession.get(),
                    state.xrSession.worldSpace_(), t,
                    state.scene_.cubes());
                if (!ok) { return {}; }
                return { reinterpret_cast<const XrCompositionLayerBaseHeader*>(&state.renderer.proj_layer()) };
            });
        }
    }

    LOG("android_main: exiting");
}
